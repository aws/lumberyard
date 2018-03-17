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

import copy
import mock
import unittest

import swagger_json_navigator
import swagger_processor.lambda_dispatch
import swagger_processor


class UnitTest_CloudGemFramework_ResourceManagerCode_swagger_processor_lambda_dispatch(unittest.TestCase):


    def __init__(self, *args, **kwargs):
        super(UnitTest_CloudGemFramework_ResourceManagerCode_swagger_processor_lambda_dispatch, self).__init__(*args, **kwargs)
        self.maxDiff = None


    def process_swagger(self, swagger):
        mock_context = mock.MagicMock()
        swagger_processor.lambda_dispatch.process_lambda_dispatch_objects(mock_context, swagger_json_navigator.SwaggerNavigator(swagger))
        swagger_processor.validate_swagger(swagger)


    def test_no_lambda_dispatch_object(self):
        swagger = self.__get_minimal_swagger()
        swagger['paths']['/test'] = {
            "get": {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            }
        }
        self.assertRaises(ValueError, self.process_swagger, swagger)


    def test_no_lambda_property(self):
        swagger = self.__get_minimal_swagger()
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {}
        swagger['paths']['/test'] = {
            "get": {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            }
        }
        self.assertRaises(ValueError, self.process_swagger, swagger)


    def test_no_lambda_dispatch_object_with_integration_object(self):
        swagger = self.__get_minimal_swagger()
        swagger['paths']['/test'] = {
            "get": {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                swagger_processor.lambda_dispatch.API_GATEWAY_INTEGRATION_OBJECT_NAME: {
                }
            }
        }
        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.process_swagger(swagger)
        self.__validate_swagger(expected_swagger, swagger)


    def test_all_operations(self):
        
        swagger = self.__get_minimal_swagger()

        lambda_arn = 'TestLambda'
        
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }

        path = '/test'
        
        test_path = swagger['paths'][path] = {}

        # all but options, which gets special handling when cors is enabled (which it is by default)
        all_operations = ['get', 'put', 'post', 'delete', 'head', 'patch']

        for operation in all_operations:
            test_path[operation] = {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            }

        test_path['x-foo'] = {} # should be ignored

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        for operation in all_operations:
            self.__add_error_responses(expected_swagger, path, operation)
            self.__add_integration_object(expected_swagger, path, operation, lambda_arn, 'test', operation)
            self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
            self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
        
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_default_expected_definitions(expected_swagger)
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_multiple_paths(self):

        swagger = self.__get_minimal_swagger()

        lambda_arn = 'TestLambda'
        
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }
        
        modules = ['testA', 'testB', 'testA']

        operation = 'get'
        function = operation

        for module in modules:
            swagger['paths']['/' + module] = {
                operation: {
                    "responses": {
                        "200": {
                            "description": "A successful service status response."
                        }
                    }
                }
            }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        for module in modules:
            path = '/' + module
            self.__add_error_responses(expected_swagger, path, operation)
            self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
            self.__add_cors_options_operation(expected_swagger['paths'][path])
            self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
            self.__add_operation_iam_security(expected_swagger['paths'][path][operation])

        self.__add_default_expected_definitions(expected_swagger)

        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_path_parameter(self):

        swagger = self.__get_minimal_swagger()

        lambda_arn = 'TestLambda'
        
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }

        path = '/foo/{test_param}/bar'
        module = 'foo_bar'
        operation = 'get'
        function = operation
        
        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                "parameters": [
                    {
                        "in": "path",
                        "name": 'test_param',
                        "type": "string"
                    }
                ]
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path], parameters=expected_swagger['paths'][path][operation]['parameters'])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])

        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_body_parameter(self):

        swagger = self.__get_minimal_swagger()

        lambda_arn = 'TestLambda'
        
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }

        path = '/foo/bar'
        module = 'foo_bar'
        operation = 'post'
        function = operation
        
        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                "parameters": [
                    {
                        "in": "body",
                        "name": 'test_param',
                        "schema": {
                            "type": "string"
                        }
                    }
                ]
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])

        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_parameter_reference(self):

        swagger = self.__get_minimal_swagger()

        lambda_arn = 'TestLambda'
        
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }

        path = '/foo/{test_param}/bar'
        module = 'foo_bar'
        operation = 'get'
        function = operation

        swagger['parameters'] = {
            'ref_key': {
                'in': 'path',
                'name': 'test_param',
                'type': 'string'
            }
        }
                
        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                "parameters": [
                    { "$ref": "#/parameters/ref_key" }
                ]
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        expected_parameters = [
            {
                'in': 'path',
                'name': 'test_param',
                'type': 'string'
            }
        ]
        self.__add_cors_options_operation(expected_swagger['paths'][path], parameters = expected_parameters)
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])

        self.process_swagger(swagger)
       
        self.__validate_swagger(expected_swagger, swagger)


    def test_multiple_parameters(self):

        swagger = self.__get_minimal_swagger()

        lambda_arn = 'TestLambda'
        
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }

        path = '/foo/{path_param_string}/{path_param_number}/bar'
        module = 'foo_bar'
        operation = 'post'
        function = operation
        
        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                "parameters": [
                    {
                        "in": "path",
                        "name": 'path_param_string',
                        "type": "string"
                    },
                    {
                        "in": "path",
                        "name": 'path_param_number',
                        "type": "number"
                    },
                    {
                        "in": "query",
                        "name": 'query_param_string',
                        "type": "string"
                    },
                    {
                        "in": "query",
                        "name": 'query_param_number',
                        "type": "number",
                        "required": False,
                        "default": 42
                    },
                    {
                        "in": "body",
                        "name": 'body_param',
                        "schema": {}
                    }
                ]
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path], parameters=expected_swagger['paths'][path][operation]['parameters'])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])

        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_parameter_override(self):

        swagger = self.__get_minimal_swagger()

        lambda_arn = 'TestLambda'
        
        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }

        path = '/foo/{path_param_a}/{path_param_b}/bar'
        module = 'foo_bar'
        operation = 'post'
        function = operation
        
        swagger['paths'][path] = {
            "parameters": [
                {
                    "in": "path",
                    "name": 'path_param_a',
                    "type": "number"
                },
                {
                    "in": "path",
                    "name": 'path_param_b',
                    "type": "string"
                },
                {
                    "in": "query",
                    "name": 'query_param',
                    "type": "string"
                }
            ],
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                "parameters": [
                    {
                        "in": "path",
                        "name": 'path_param_a',
                        "type": "string"
                    },
                    {
                        "in": "body",
                        "name": 'body_param',
                        "schema": {}
                    }
                ]
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path], parameters = [])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])

        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_lambda_in_swagger_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_lambda_in_path_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': 'wrong lambda arn'
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                "lambda": lambda_arn
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_lambda_in_operation_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': 'wrong lambda arn'
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                    "lambda": lambda_arn
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                "lambda": "wrong lambda"
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])

        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_module_in_swagger_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'foo_bar'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'module': module
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_module_in_path_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'foo_bar'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'module': 'wrong module'
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                "module": module
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_module_in_operation_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'foo_bar'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'module': 'wrong module'
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                    "module": module
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                "module": "wrong module"
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_function_in_swagger_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = 'foo_bar'
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'function': function
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_function_in_path_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = 'foo_bar'
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'function': 'wrong function'
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                "function": function
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_function_in_operation_object(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = 'foo_bar'
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'function': 'wrong function'
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                    "function": function
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                "function": "wrong function"
            }
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_additional_properties(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'additional-properties': {
                'a': 'aa'
            }
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                    'additional-properties': {
                        'c': 'cc'
                    }
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                'additional-properties': {
                    'b': 'bb'
                }            
            }
        }

        additional_properties = {
            'a': 'aa',
            'b': 'bb',
            'c': 'cc'
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function, additional_properties = additional_properties)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def test_additional_request_template_content(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'additional-request-template-content': 'aa'
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                    'additional-request-template-content': 'cc'
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                'additional-request-template-content': 'bb'
            }
        }

        additional_request_template_content = 'aabbcc'

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function, additional_request_template_content = additional_request_template_content)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)



    def test_additional_response_template_content(self):

        lambda_arn = 'TestLambda'
        path = '/test'
        operation = 'get'
        module = 'test'
        function = operation
        
        swagger = self.__get_minimal_swagger()

        swagger[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = {
            'lambda': lambda_arn,
            'additional-response-template-content': {
                '200': '200aa',
                '400': '400aa',
                '500': '500aa'
            }
        }

        swagger['paths'][path] = {
            operation: {
                "responses": {
                    "200": {
                        "description": "A successful service status response."
                    }
                },
                swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                    'additional-response-template-content': {
                        '200': '200cc',
                        '400': '400cc',
                        '500': '500cc'
                    }
                }
            },
            swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME: {
                'additional-response-template-content': {
                    '200': '200bb',
                    '400': '400bb',
                    '500': '500bb'
                }
            }
        }

        additional_response_template_content = {
            '200': '200aa200bb200cc',
            '400': '400aa400bb400cc',
            '500': '500aa500bb500cc',
        }

        expected_swagger = self.__copy_and_remove_lambda_dispatch_objects(swagger)
        self.__add_error_responses(expected_swagger, path, operation)
        self.__add_integration_object(expected_swagger, path, operation, lambda_arn, module, function, additional_response_template_content = additional_response_template_content)
        self.__add_default_expected_definitions(expected_swagger)
        self.__add_cors_options_operation(expected_swagger['paths'][path])
        self.__add_cors_response_headers(expected_swagger['paths'][path][operation])
        self.__add_operation_iam_security(expected_swagger['paths'][path][operation])
    
        self.process_swagger(swagger)
        
        self.__validate_swagger(expected_swagger, swagger)


    def __get_minimal_swagger(self):
        # return a new copy every time so it can be modified
        return {
            "swagger": "2.0",
            "info": {
                "version": "1.0.0",
                "title": "TestTitle",
            },
            "paths": {
            }
        }


    def __validate_swagger(self, expected_swagger, actual_swagger):
        print '******* expected swagger *******'
        print expected_swagger
        print '******** actual swagger ********'
        print actual_swagger
        print '********************************'

        self.assertDictEqual(expected_swagger, actual_swagger)


    def __copy_and_remove_lambda_dispatch_objects(self, swagger):
        swagger_copy = copy.deepcopy(swagger)
        self.__remove_lambda_dispatch_object(swagger_copy)
        for path in swagger_copy['paths'].values():
            self.__remove_lambda_dispatch_object(path)
            for operation in path.values():
                self.__remove_lambda_dispatch_object(operation)
        return swagger_copy


    def __remove_lambda_dispatch_object(self, object):
        if swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME in object:
            del object[swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME]


    def __add_error_responses(self, swagger, path, operation):
        operation_object = swagger['paths'][path][operation]
        operation_object['responses']['400'] = {
            "description": swagger_processor.lambda_dispatch.RESPONSE_DESCRIPTION_400,
            "schema": swagger_processor.lambda_dispatch.ERROR_RESPONSE_SCHEMA_REF
        }
        operation_object['responses']['403'] = {
            "description": swagger_processor.lambda_dispatch.RESPONSE_DESCRIPTION_403,
            "schema": swagger_processor.lambda_dispatch.ERROR_RESPONSE_SCHEMA_REF
        }
        operation_object['responses']['404'] = {
            "description": swagger_processor.lambda_dispatch.RESPONSE_DESCRIPTION_404,
            "schema": swagger_processor.lambda_dispatch.ERROR_RESPONSE_SCHEMA_REF
        }
        operation_object['responses']['500'] = {
            "description": swagger_processor.lambda_dispatch.RESPONSE_DESCRIPTION_500,
            "schema": swagger_processor.lambda_dispatch.ERROR_RESPONSE_SCHEMA_REF
        }


    class __RequestTemplateMatcher(object):

        
        def __init__(self, module, function, parameters, additional_content):
            self.__module = module
            self.__function = function
            self.__parameters = parameters
            self.__additional_content = additional_content

        
        def __eq__(self, template):

            if not isinstance(template, str):
                raise AssertionError('Request template type is not string: {}'.format(template))

            if '"module":"{}"'.format(self.__module) not in template:
                raise AssertionError('Request template does not specify {} module: {}'.format(self.__module, template))

            if '"function":"{}"'.format(self.__function) not in template:
                raise AssertionError('Request template does not specify {} function: {}'.format(self.__function, template))

            if self.__parameters:
                if 'parameters' not in template:
                    raise AssertionError('Request template does not contain parameters when they were expected: {}'.format(template))
                for parameter_object in self.__parameters:
                    parameter_name = parameter_object['name']
                    parameter_location = parameter_object['in']
                    parameter_type = parameter_object.get('type', None)
                    parameter_default = parameter_object.get('default', None)
                    if parameter_location == 'body':
                        if '''$input.json('$')''' not in template:
                            raise AssertionError('Request template does not contain body parameter {}: {}'.format(parameter_name, template))
                    elif parameter_location == 'path':
                        if parameter_type == 'string':
                            if '$util.escapeJavaScript' not in template:
                                raise AssertionError('Request template does not include escapeJavaScript for string parameter {}: {}'.format(parameter_name, template))
                            if '$util.urlDecode' not in template:
                                raise AssertionError('Request template does not include urlDecode for path parameter {}: {}'.format(parameter_name, template))
                        if '''$input.params().get('path').get('{}')'''.format(parameter_name) not in template:
                            raise AssertionError('Request template does not contain accessor for parameter {}: {}'.format(parameter_name, template))
                    elif parameter_location == 'query':
                        if parameter_type == 'string':
                            if '$util.escapeJavaScript' not in template:
                                raise AssertionError('Request template does not include escapeJavaScript for string parameter {}: {}'.format(parameter_name, template))
                        if '''$input.params().get('querystring').get('{}')'''.format(parameter_name) not in template:
                            raise AssertionError('Request template does not contain accessor for parameter {}: {}'.format(parameter_name, template))
                        if parameter_default is not None:
                            if str(parameter_default) not in template:
                                raise AssertionError('Request template does not contain default value for parameter {}: {}'.format(parameter_name, template))
                    else:
                        raise AssertionError('Unsupported parameter location: {}'.format(parameter_location))
            elif 'parameters' in template:
                raise AssertionError('Request template contains parameters when none expected: {}'.format(template))

            if self.__additional_content:
                if self.__additional_content not in template:
                    raise AssertionError('Request template does not contain {} additional content: {}'.format(self.__additional_content, template))

            return True


    class __ErrorResponseTemplateMatcher(object):


        def __init__(self, additional_content):
            self.__additional_content = additional_content


        def __eq__(self, template):

            if not isinstance(template, str):
                raise AssertionError('Error template type is not string: {}'.format(template))

            if '"errorMessage":' not in template:
                raise AssertionError('Error template does not contain errorMessage: {}'.format(template))

            if '"errorType":' not in template:
                raise AssertionError('Error template does not contain errorTYpe: {}'.format(template))

            if self.__additional_content:
                if self.__additional_content not in template:
                    raise AssertionError('Error template does not contain {} additional content: {}'.format(self.__additional_content, template))

            return True


    class __SuccessResponseTemplateMatcher(object):


        def __init__(self, additional_content):
            self.__additional_content = additional_content


        def __eq__(self, template):

            if not isinstance(template, str):
                raise AssertionError('Success template type is not string: {}'.format(template))

            if "\"result\":$input.json('$')" not in template:
                raise AssertionError('Success template does not contain result: {}'.format(template))

            if self.__additional_content:
                if self.__additional_content not in template:
                    raise AssertionError('Success template does not contain {} additional content: {}'.format(self.__additional_content, template))

            return True


    def __add_integration_object(self, swagger, path, operation, lambda_arn, module, function, additional_properties = None, additional_request_template_content = None, additional_response_template_content = None, cors_enabled = True):

        path_object = swagger['paths'][path]
        operation_object = path_object[operation]

        # Start with the operation's parameters, then add any path paramters
        # that were not specified by the operation
        parameters = copy.copy(operation_object.get('parameters', []))
        parameters.extend(path_object.get('parameters', []))
        parameters_seen = set()
        actual_parameters = []
        for parameter in parameters:
            if '$ref' in parameter:
                ref = parameter['$ref'] # should be '#/parameters/name'
                ref_parts = ref.split('/')
                ref_name = ref_parts[2]
                parameter = swagger['parameters'][ref_name]
            paramter_name = parameter['name']
            if paramter_name not in parameters_seen:
                actual_parameters.append(parameter)
                parameters_seen.add(paramter_name)

        if not additional_response_template_content: additional_response_template_content = {}

        integration_object = {
            "credentials": "$RoleArn$", 
            "requestTemplates": {
                "application/json": self.__RequestTemplateMatcher(module, function, actual_parameters, additional_request_template_content)
            }, 
            "responses": {
                "(?s)(?!Client Error:|Forbidden:|Not Found:).+": {
                    "responseTemplates": {
                        "application/json": self.__ErrorResponseTemplateMatcher(additional_response_template_content.get('500', None))
                    }, 
                    "statusCode": "500"
                }, 
                "(?s)Client Error:.*": {
                    "responseTemplates": {
                        "application/json": self.__ErrorResponseTemplateMatcher(additional_response_template_content.get('400', None))
                    }, 
                    "statusCode": "400"
                }, 
                "(?s)Forbidden:.*": {
                    "responseTemplates": {
                        "application/json": self.__ErrorResponseTemplateMatcher(additional_response_template_content.get('403', None))
                    }, 
                    "statusCode": "403"
                }, 
                "(?s)Not Found:.*": {
                    "responseTemplates": {
                        "application/json": self.__ErrorResponseTemplateMatcher(additional_response_template_content.get('404', None))
                    }, 
                    "statusCode": "404"
                }, 
                "default": {
                    "responseTemplates": {
                        "application/json": self.__SuccessResponseTemplateMatcher(additional_response_template_content.get('200', None))
                    }, 
                    "statusCode": "200"
                }
            }, 
            "type": "AWS", 
            "uri": "arn:aws:apigateway:$Region$:lambda:path/2015-03-31/functions/{}/invocations".format(lambda_arn),
            "httpMethod": "POST",
            'passthroughBehavior': 'never'
        }

        if additional_properties:
            integration_object.update(additional_properties)

        if cors_enabled:
            for response in integration_object['responses']:
                integration_object['responses'][response]['responseParameters'] = {
                    "method.response.header.Access-Control-Allow-Origin": "'*'"
                }

        operation_object[swagger_processor.lambda_dispatch.API_GATEWAY_INTEGRATION_OBJECT_NAME] = integration_object
        
    def __add_default_expected_definitions(self, swagger):
        self.__add_error_definition_object(swagger)
        self.__add_empty_response_definition_object(swagger)
        self.__add_iam_security_definition(swagger)

    def __add_error_definition_object(self, swagger):
        self.__add_definition_object(swagger, 'Error', swagger_processor.lambda_dispatch.ERROR_RESPONSE_SCHEMA)

    def __add_empty_response_definition_object(self, swagger):
        self.__add_definition_object(swagger, 'EmptyResponse', swagger_processor.lambda_dispatch.EMPTY_RESPONSE_SCHEMA)

    def __add_definition_object(self, swagger, name, definition):

        definitions = swagger.get('definitions', None)
        if not definitions:
            definitions = swagger['definitions'] = {}

        if name not in definitions:
            definitions[name] = definition

    def __add_cors_options_operation(self, path, parameters = []):

        filtered_parameters = []
        for parameter in parameters:
            if parameter['in'] == 'path':
                filtered_parameters.append(
                    {
                        'name': parameter['name'],
                        'in': 'path',
                        'type': 'string'
                    }
                )

        path['options'] = {
            "consumes": [
                "application/json"
            ],
            "produces": [
                "application/json"
            ],
            "parameters": filtered_parameters,
            "responses": {
                "200": {
                    "description": "200 response",
                    "schema": {
                        "$ref": "#/definitions/EmptyResponse"
                    },
                    "headers": {
                        "Access-Control-Allow-Origin": {
                            "type": "string"
                        },
                        "Access-Control-Allow-Methods": {
                            "type": "string"
                        },
                        "Access-Control-Allow-Headers": {
                            "type": "string"
                        }
                    }
                }
            },
            "x-amazon-apigateway-integration": {
                "responses": {
                    "default": {
                        "statusCode": "200",
                        "responseParameters": {
                            "method.response.header.Access-Control-Allow-Methods": "'POST,GET,DELETE,PUT,HEAD,PATCH,OPTIONS'",
                            "method.response.header.Access-Control-Allow-Headers": "'Content-Type,X-Amz-Date,Authorization,X-Api-Key,X-Amz-Security-Token'",
                            "method.response.header.Access-Control-Allow-Origin": "'*'"
                        }
                    }
                },
                "passthroughBehavior": "when_no_match",
                "requestTemplates": {
                    "application/json": "{\"statusCode\": 200}"
                },
                "type": "mock"
            }
        }

    def __add_cors_response_headers(self, operation):
        for code in ['200', '400', '403', '404', '500']:
            operation['responses'][code]['headers'] = {
                'Access-Control-Allow-Origin': {'type': 'string'}
            }

    def __add_operation_iam_security(self, operation):
        operation['security'] = [
            {
                "sigv4": []
            }
        ]

    def __add_iam_security_definition(self, swagger):
        swagger['securityDefinitions'] = {
            "sigv4": {
                "type": "apiKey",
                "name": "Authorization",
                "in": "header",
                "x-amazon-apigateway-authtype": "awsSigv4"
            }
        }
