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

LAMBDA_DISPATCH_OBJECT_NAME = 'x-amazon-cloud-canvas-lambda-dispatch'
API_GATEWAY_INTEGRATION_OBJECT_NAME = 'x-amazon-apigateway-integration'

RESPONSE_DESCRIPTION_400 = "Response indicating the client's request was invalid."
RESPONSE_DESCRIPTION_403 = "Response indicating the client's request was not authenticated or authorized."
RESPONSE_DESCRIPTION_404 = "Response indicating the resource requested by the client does not exist."
RESPONSE_DESCRIPTION_500 = "Response indicating the service encountered an internal error when processing the request."
ERROR_RESPONSE_SCHEMA_REF = { "$ref": "#/definitions/Error" }
ERROR_RESPONSE_SCHEMA = {
    "type": "object",
    "properties": {
        "errorCode": {
            "type": "string"
        },
        "errorMessage": {
            "type": "string"
        }
    },
    "required": [
        "errorCode",
        "errorMessage"
    ]
}
EMPTY_RESPONSE_SCHEMA = {
}

def process_lambda_dispatch_objects(context, swagger_navigator):

    # The x-amazon-cloud-canvas-lambda-dispatch objects can appear in the swagger object (root)
    # a path object or an operation object. We use these to generate x-amazon-apigateway-integration
    # objects for each operation that doesn't have one already.
    #
    # For the lambda, module, and function properties, the value specified by the "lowest" object 
    # override any values provided by the "higher" object (so you can set a defaults at the top 
    # and override then for individual paths or operations).
    #
    # For additional_properties, additional_request_template_content, and additional_response_template_content
    # properties, the aggregate values are injected into the x-amazon-apigateway-integration.
    #
    # We keep track of the lambda dispatch objects that are "in scope" in the displatch_object_stack.
    # This starts with the one in the swagger object, then the one for a "current" path, then
    # one for the "current" operation (the stack will never have more than three entries).
    #
    # As part of this processing, we need to know an operation's current parameters. Swagger 
    # allows parameters to be defined in the path or operation object, with parameters in the
    # operation object overriding those in the path object. Swagger lets you put paramter 
    # definitions in the swagger object, but these are not used for any path/operation unless
    # the path/operation parameters use $ref to identify one of these definitions. We use the
    # parameter_object_stack to keep track of the "current" path and operation parameters. 

    dispatch_object_stack = []
    parameters_object_stack = []

    dispatch_object_stack.append(swagger_navigator.remove_object(LAMBDA_DISPATCH_OBJECT_NAME, {}))

    missing_security_warnings = []

    global_security_object = swagger_navigator.get_object('security', default=None)

    for path, path_object in swagger_navigator.get_object('paths').items():

        dispatch_object_stack.append(path_object.remove_object(LAMBDA_DISPATCH_OBJECT_NAME, {}))
        parameters_object_stack.append(path_object.get_array('parameters', []))

        options_operation_needed_for_cors = False
        options_operation_found = False

        for operation, operation_object in path_object.items():

            # Swagger allows a path object to have properties that don't represent an 
            # operation. Only the following properties define operations.

            if operation not in ['get', 'put', 'post', 'delete', 'options', 'head', 'patch']:
                continue

            if operation == 'options':
                options_operation_found = True
                
            dispatch_object_stack.append(operation_object.remove_object(LAMBDA_DISPATCH_OBJECT_NAME, {}))
            parameters_object_stack.append(operation_object.get_array('parameters', []))

            # Create an x-amazon-apigateway-intergration object only if the operation object 
            # doesn't have one already.

            if not operation_object.contains(API_GATEWAY_INTEGRATION_OBJECT_NAME):

                # We assume executing a lambda can result in both internal server errors and
                # client errors, in addition to success. We add definitions for these 
                # responses to the operation object if they aren't already present.
                if _add_error_response(operation_object):
                    _ensure_error_definition(swagger_navigator)

                # By default we want all APIs to be callable only when using valid AWS IAM 
                # credentails. If no security object is present, add add one. 
                if _determine_if_iam_security_enabled(dispatch_object_stack):
                    if _add_iam_security_to_operation(operation_object):
                        _ensure_iam_security_definition(swagger_navigator)

                # Construct the x-amazon-apigateway-intergration object using the information
                # we have in the x-amazon-cloud-canvas-lambda-dispatch objects that are currently
                # in scope.
                integration_object = _make_integration_object(dispatch_object_stack, parameters_object_stack, path, operation)
                operation_object.value[API_GATEWAY_INTEGRATION_OBJECT_NAME] = integration_object

                if _determine_if_cors_enabled(dispatch_object_stack):
                    options_operation_needed_for_cors = True
                    __add_cores_response_headers_to_operation(operation_object)

            # If no security has been declared or inserted above, API Gateway will make the operation public.
            if global_security_object.is_none:
                security_array = operation_object.get_array('security', default = None)
                if security_array.is_none:
                    missing_security_warnings.append('    {:<7} {}'.format(operation, path))

            dispatch_object_stack.pop() # operation scope
            parameters_object_stack.pop()

        if options_operation_needed_for_cors and not options_operation_found:
            __add_options_operation_for_cors(path, path_object)

        dispatch_object_stack.pop() # path scope
        parameters_object_stack.pop()

    dispatch_object_stack.pop() # swagger scope

    if missing_security_warnings:
        print ''
        print 'WARNING: the following operations do not specify a swagger "security" object.'
        print 'The Service APIs for these operations will be publically accessible.'
        print ''
        for warning in missing_security_warnings:
            print warning
        print ''


def _add_error_response(operation_object):

    responses_object = operation_object.get_object('responses')

    added = False

    if not responses_object.contains('400'):
        responses_object.value['400'] = {
            "description": RESPONSE_DESCRIPTION_400,
            "schema": copy.deepcopy(ERROR_RESPONSE_SCHEMA_REF)
        }
        added = True

    if not responses_object.contains('403'):
        responses_object.value['403'] = {
            "description": RESPONSE_DESCRIPTION_403,
            "schema": copy.deepcopy(ERROR_RESPONSE_SCHEMA_REF)
        }
        added = True

    if not responses_object.contains('404'):
        responses_object.value['404'] = {
            "description": RESPONSE_DESCRIPTION_404,
            "schema": copy.deepcopy(ERROR_RESPONSE_SCHEMA_REF)
        }
        added = True

    if not responses_object.contains('500'):
        responses_object.value['500'] = {
            "description": RESPONSE_DESCRIPTION_500,
            "schema": copy.deepcopy(ERROR_RESPONSE_SCHEMA_REF)
        }
        added = True

    return added


def _ensure_error_definition(swagger_object):
    definitions_object = swagger_object.get_or_add_object('definitions')
    if not definitions_object.contains('Error'):
        definitions_object.value['Error'] = ERROR_RESPONSE_SCHEMA


def _ensure_empty_response_definition(swagger_object):
    definitions_object = swagger_object.get_or_add_object('definitions')
    if not definitions_object.contains('EmptyResponse'):
        definitions_object.value['EmptyResponse'] = EMPTY_RESPONSE_SCHEMA


def __add_cores_response_headers_to_operation(operation_object):
    responses = operation_object.get_or_add_object('responses')
    __add_cores_header_to_object(responses.get_or_add_object('200'), 'Access-Control-Allow-Origin', 'string')
    __add_cores_header_to_object(responses.get_or_add_object('400'), 'Access-Control-Allow-Origin', 'string')
    __add_cores_header_to_object(responses.get_or_add_object('403'), 'Access-Control-Allow-Origin', 'string')
    __add_cores_header_to_object(responses.get_or_add_object('404'), 'Access-Control-Allow-Origin', 'string')
    __add_cores_header_to_object(responses.get_or_add_object('500'), 'Access-Control-Allow-Origin', 'string')

def __add_cores_header_to_object(object, headername, headertype):
    headers = object.get_or_add_object('headers')
    access_control_allow_orgin_header = headers.get_or_add_object(headername)
    access_control_allow_orgin_header.value['type'] = headertype

def __add_options_operation_for_cors(path, path_object):
    
    _ensure_empty_response_definition(path_object.root)

    # need to declare any path paramters that are not declared in the path object itself
    options_parameters = []
    path_parameters = path_object.get_array('parameters', default = [])
    path_parts = path.split('/')
    for path_part in path_parts:
        if path_part.startswith('{'):
            parameter_name = path_part[1:-1]
            found = False
            for path_parameter in path_parameters.values():
                if path_parameter.get_string('name').value == parameter_name:
                    found = True
                    break
            if not found:
                options_parameters.append(
                    {
                        "in": "path",
                        "name": parameter_name,
                        "type": "string"
                    }
                )

    path_object.add_object('options',
        {
            "consumes": [
                "application/json"
            ],
            "produces": [
                "application/json"
            ],
            "parameters": options_parameters,
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
    )

           
def _add_iam_security_to_operation(operation_object):

    added = False

    if not operation_object.contains('security'):

        operation_object.value['security'] = [
            {
                "sigv4": []
            }
        ]

        added = True

    return added


def _ensure_iam_security_definition(swagger_object):
    security_definitions_object = swagger_object.get_or_add_object('securityDefinitions')
    if not security_definitions_object.contains('sigv4'):
        security_definitions_object.value['sigv4'] = {
            "type": "apiKey",
            "name": "Authorization",
            "in": "header",
            "x-amazon-apigateway-authtype": "awsSigv4"
        }


def _make_integration_object(dispatch_object_stack, parameters_object_stack, path, operation):

    # Determine the data that will go into the x-amazon-apigateway-integration object
    # using the x-amazon-cloud-canvas-lambda-dispatch objects.

    lambda_arn = _determine_lambda(dispatch_object_stack)
    module = _determine_module(dispatch_object_stack, path)
    function = _determine_function(dispatch_object_stack, operation)
    additional_properties = _determine_additional_properties(dispatch_object_stack)
    additional_request_template_content = _determine_additional_request_template_content(dispatch_object_stack)
    additional_response_template_content = _determine_additional_response_template_content(dispatch_object_stack)

    # The API Gateway specified format for identifing the lambda to execute (I wonder
    # why a normal lambda ARN wasn't sufficient).

    uri = "arn:aws:apigateway:$Region$:lambda:path/2015-03-31/functions/{}/invocations".format(lambda_arn)

    # The request template tells API Gateway how to map data in an incomming request
    # to the content sent to the labmda function for processing. This includes the
    # module and function name that the dispatcher in the lambda will use to satisfy
    # the request. It also includes all the parameter values specified by the operation, 
    # which will also include the body for some operations.

    request_template = _make_request_template(path, operation, module, function, parameters_object_stack, additional_request_template_content)

    # The response templates tell API Gateway hot to map data in a response from lambda
    # to the response sent to the client. For successful responses, this includes the body
    # of the lambda response. For client errors, the error message returned by the lambda
    # is forwarded on to the client. For service errors, an generic message is returned to
    # the client (to prevent the service from leaking sensative information). The stack trace
    # is never sent to the client.

    response_template_200 = "{{\"result\":$input.json('$'){}}}".format(additional_response_template_content.get('200', ''))
    response_template_500 = "{{\"errorMessage\":\"Service Error: An internal service error has occurred.\",\"errorType\":\"ServiceError\"{}}}".format(additional_response_template_content.get('500', ''))
    response_template_400 = "{{\"errorMessage\":$input.json('$.errorMessage'),\"errorType\":$input.json('$.errorType'){}}}".format(additional_response_template_content.get('400', ''))
    response_template_403 = "{{\"errorMessage\":$input.json('$.errorMessage'),\"errorType\":$input.json('$.errorType'){}}}".format(additional_response_template_content.get('403', ''))
    response_template_404 = "{{\"errorMessage\":$input.json('$.errorMessage'),\"errorType\":$input.json('$.errorType'){}}}".format(additional_response_template_content.get('404', ''))

    # Construct and return the x-amazon-apigateway-integration object.

    integration_object = {
        "type": "AWS",
        "uri": uri,
        "httpMethod": "POST",
        "credentials": "$RoleArn$",
        "passthroughBehavior": "never",
        "requestTemplates": {
            "application/json": request_template
        },
        "responses": {

            "default": {
                "statusCode": "200",
                "responseTemplates": {
                    "application/json": response_template_200
                }
            },

            # This one matches all non-empty error strings except for the ones matched
            # by the expressions below. Note that this pattern has only match non-empty
            # strings (done by the + on the end), otherwise it will match success 
            # responses as well.
            #
            # Also note the use of (?s) to enable DOTALL mode, which casues . to 
            # match new lines. See https://docs.oracle.com/javase/8/docs/api/java/util/regex/Pattern.html#DOTALL.

            "(?s)(?!Client Error:|Forbidden:|Not Found:).+": {
                "statusCode": "500",
                "responseTemplates": {
                    "application/json": response_template_500
                }
            },

            # The prefixes in these strings correspond to the prefixes used by the
            # exception classes defined in the errors.py file.

            "(?s)Client Error:.*": {
                "statusCode": "400",
                "responseTemplates": {
                    "application/json": response_template_400
                }
            },
            "(?s)Forbidden:.*": {
                "statusCode": "403",
                "responseTemplates": {
                    "application/json": response_template_403
                }
            },
            "(?s)Not Found:.*": {
                "statusCode": "404",
                "responseTemplates": {
                    "application/json": response_template_404
                }
            }

        }
    }

    _merge_properties(integration_object, additional_properties)

    if _determine_if_cors_enabled(dispatch_object_stack):        
        for response in integration_object['responses']:            
            integration_object['responses'][response]['responseParameters'] = {
                 "method.response.header.Access-Control-Allow-Origin": "'*'"
            }


    return integration_object


def _determine_lambda(dispatch_object_stack):

    # Assumes the stack contains dispatch objects from the following scopes in the 
    # specified order:
    #
    #    operation (top)
    #    path
    #    swagger   (bottom)
    # 
    # "lambda" property values from objects toward the top of the stack override those toward 
    # the bottom of the stack. At least one object has to provide a value for this property.

    lambda_arn = _get_top_of_stack_string_property(dispatch_object_stack, 'lambda')

    if not lambda_arn:
        raise ValueError('No lambda property found in x-amazon-cloud-canvas-lambda-dispatch objects.')

    return lambda_arn


def _determine_module(dispatch_object_stack, path):

    # Assumes the stack contains dispatch objects from the following scopes in the 
    # specified order:
    #
    #    operation (top)
    #    path
    #    swagger   (bottom)
    # 
    # "model" property values from objects toward the top of the stack override those toward 
    # the bottom of the stack. A default value is constructed if none is provided.

    module = _get_top_of_stack_string_property(dispatch_object_stack, 'module')

    if not module:
        module = get_default_module_name_for_path(path)
                
    return module


def get_default_module_name_for_path(path):

    path_parts = path.split('/')
    if len(path_parts) < 1 or len(path_parts[0]) > 0:
        raise ValueError('Unsupported swagger path: {}'.format(path))

    module = ''

    for path_part in path_parts[1:]:
        if not path_part.startswith('{'):
            if module: 
                module += '_'
            module += path_part

    if not module: 
        module = 'root'

    return module



def _determine_function(dispatch_object_stack, operation):

    # Assumes the stack contains dispatch objects from the following scopes in the 
    # specified order:
    #
    #    operation (top)
    #    path
    #    swagger   (bottom)
    # 
    # "function" property values from objects toward the top of the stack override those toward 
    # the bottom of the stack. A default value is constructed if none is provided.

    function = _get_top_of_stack_string_property(dispatch_object_stack, 'function')

    if not function:
        function = operation        

    return function


def _determine_additional_properties(dispatch_object_stack):

    # Assumes the stack contains dispatch objects from the following scopes in the 
    # specified order:
    #
    #    operation (top)
    #    path
    #    swagger   (bottom)
    # 
    # "additional_properties" property values are aggregated together, starting with 
    # the values at the bottom of the stack.

    additional_properties = {}

    for dispatch_object in dispatch_object_stack:
        _merge_properties(additional_properties, dispatch_object.get_object('additional-properties', {}).value)

    return additional_properties


def _determine_additional_request_template_content(dispatch_object_stack):

    # Assumes the stack contains dispatch objects from the following scopes in the 
    # specified order:
    #
    #    operation (top)
    #    path
    #    swagger   (bottom)
    # 
    # "additional_request_template_content" property values are aggregated together, 
    # starting with the values at the bottom of the stack.

    additional_request_template_content = ''

    for dispatch_object in dispatch_object_stack:
        additional_request_template_content += dispatch_object.get_string('additional-request-template-content', '').value

    return additional_request_template_content


def _determine_additional_response_template_content(dispatch_object_stack):

    # Assumes the stack contains dispatch objects from the following scopes in the 
    # specified order:
    #
    #    operation (top)
    #    path
    #    swagger   (bottom)
    # 
    # "additional_response_template_content" property values are aggregated together, 
    # starting with the values at the bottom of the stack.

    additional_response_template_content = {}

    for dispatch_object in dispatch_object_stack:
        for key, navigator in dispatch_object.get_object('additional-response-template-content', {}).items():
            if key in additional_response_template_content:
                additional_response_template_content[key] += navigator.value
            else:
                additional_response_template_content[key] = navigator.value

    return additional_response_template_content


def _determine_if_cors_enabled(dispatch_object_stack):
    return _get_top_of_stack_boolean_property(dispatch_object_stack, 'enable_cores', default_value = True)

def _determine_if_iam_security_enabled(dispatch_object_stack):
    return _get_top_of_stack_boolean_property(dispatch_object_stack, 'enable_iam', default_value = True)

def _get_top_of_stack_string_property(dispatch_object_stack, property_name):
    # Items are appended to the stack, so traverse the list in reverse to 
    # find the first instance of the specified property.
    for dispatch_object in reversed(dispatch_object_stack):
        navigator = dispatch_object.get_string(property_name, None)
        if not navigator.is_none: return navigator.value
    return None


def _get_top_of_stack_boolean_property(dispatch_object_stack, property_name, default_value = None):
    # Items are appended to the stack, so traverse the list in reverse to 
    # find the first instance of the specified property.
    for dispatch_object in reversed(dispatch_object_stack):
        navigator = dispatch_object.get_boolean(property_name, None)
        if not navigator.is_none: return navigator.value
    return default_value


def _merge_properties(destination, source):
    for key, value in source.iteritems():
        if key in destination and isinstance(value, dict):
            _merge_properties(destination[key], value)
        else:
            destination[key] = value


def _make_request_template(path, operation, module, function, parameters_object_stack, additional_request_template_content):

    has_parameters = _has_parameters(parameters_object_stack)
    not_a_dangling_comma = ',' if has_parameters else ''

    request_template = ''

    request_template += '{\n'
    request_template += '  \"module\":\"{}\",\n'.format(module)
    request_template += '  \"function\":\"{}\",\n'.format(function)
    request_template += '  \"cognitoIdentityId\":\"$context.identity.cognitoIdentityId\",\n'
    request_template += '  \"cognitoIdentityPoolId\":\"$context.identity.cognitoIdentityPoolId\",\n'
    request_template += '  \"sourceIp\":\"$context.identity.sourceIp\"{comma}\n'.format(comma=not_a_dangling_comma)

    if has_parameters:

        request_template += '  \"parameters\": {\n'

        # Assumes the stack contains parameters arrays from the following scopes in the 
        # specified order:
        #
        #    operation (top)
        #    path      (bottom)
        #
        # The operation array is processed first, then the path array. References are
        # resolved. The parameters_seen set keeps track of the name/in combinations seen 
        # so far and is used to prevent path parameter definitions from overriding operation 
        # prameter definitions.
        #
        # We then check to see if any of the parameter names conflict independently of
        # location. This is a Cloud Canvas imposed limitation.

        parameters_seen = set()
        actual_parameters = []
        for parameters_object in reversed(parameters_object_stack):
            for parameter_object in parameters_object.values():
                if parameter_object.is_ref:
                    parameter_object = parameter_object.resolve_ref()
                seen_key = parameter_object.get_string('in').value + ':' + parameter_object.get_string('name').value
                if seen_key not in parameters_seen:
                    actual_parameters.append(parameter_object)
                    parameters_seen.add(seen_key)

        parameters_seen = set()
        for parameter_object in actual_parameters:
            parameter_name = parameter_object.get_string('name').value
            if parameter_name in parameters_seen:
                raise ValueError('Path {} operation {} defines multiple {} parameters. Cloud Canvas does not support parameters with the same name even when they come from different locations.'.format(
                    path,
                    operation,
                    parameter_name))
            parameters_seen.add(parameter_name)
            last_parameter = len(parameters_seen) == len(actual_parameters)
            request_template += _get_parameter_request_template_mapping(path, operation, parameter_object, last_parameter)

        request_template += '  }\n'

    # should have a leading comma
    if additional_request_template_content:
        request_template += additional_request_template_content

    request_template += '}\n'

    return request_template


def _get_parameter_request_template_mapping(path, operation, parameter_object, last_parameter):
    
    if parameter_object.is_ref:
        parameter_object = parameter_object.resolve_ref()

    location = parameter_object.get_string('in').value

    if location == 'body':
        return _get_body_parameter_request_template_mapping(path, operation, parameter_object, last_parameter)
    elif location == 'path':
        return _get_path_parameter_request_template_mapping(path, operation, parameter_object, last_parameter)
    elif location == 'query':
        return _get_query_parameter_request_template_mapping(path, operation, parameter_object, last_parameter)
    else:
        raise ValueError('Path {} operation {} defines {} parameter with location {}. Cloud Canvas does not support parameters with this location.'.format(
            path,
            operation,
            parameter_object.get_string('name').value,
            location))


BODY_PARAMETER_REQUEST_TEMPLATE_MAPPING = '''
#if($input.body != '{{}}')
    "{name}": $input.json('$'){comma}
#else
    "{name}": null{comma}
#end
'''

def _get_body_parameter_request_template_mapping(path, operation, parameter_object, last_parameter):

    # Swagger allows the body parameter to specify required = false but it does
    # not allow a default value to be povided. API Gateway seems to use the 
    # application/json mapping for POST requests without any content type or 
    # body. 
    # 
    # The correct behavior here would be to check the content type and the body 
    # length to determine if the body parameter was not provided. Unfortunatly 
    # the test UI in the AWS console doesn't let you specify a content type (it 
    # always uses ""). So if we implement the correct behavior, the APIs can't
    # be tested using the AWS console, which would be unfortunate.
    #
    # API Gateway also uses "{}" as the body when no body was specified, so
    # this prevents distingisuing between an missing body and empty object
    # impossible, when technically they are two very different things.
    #
    # So, for lack of any other alturnative, we check for an {} body and 
    # pass along null if that is what we get. 

    # So we check the content type and pass allong a null parameter value if it 
    # isn't provided and the lambda will check that all required parameters
    # are provided. Otherwise, the body content will be inserted into the message 
    # sent to the Lambda function without additional processing.

    not_a_dangling_comma = ',' if not last_parameter else ''
    name = parameter_object.get_string('name').value

    if operation in ['head', 'get']:
        raise ValueError('Path {} operation {} defines {} parameter with location body. Cloud Canvas does not body parameters with this operation.'.format(
            path,
            operation,
            name))
    
    return BODY_PARAMETER_REQUEST_TEMPLATE_MAPPING.format(name=name, comma=not_a_dangling_comma)


PATH_PARAMETER_REQUEST_TEMPLATE_MAPPING_STRING = '''
#if($input.params().get('path').keySet().contains('{name}'))
    "{name}": "$util.escapeJavaScript($util.urlDecode($input.params().get('path').get('{name}'))).replaceAll("\\\\'","'")"{comma}
#else
    "{name}": null{comma}
#end    
'''

PATH_PARAMETR_REQUEST_TEMPLATE_MAPPING_NOT_STRING = '''
#if($input.params().get('path').keySet().contains('{name}'))
    "{name}": $input.params().get('path').get('{name}'){comma}
#else
    "{name}": null{comma}
#end    
'''

def _get_path_parameter_request_template_mapping(path, operation, parameter_object, last_parameter):

    # Swagger says that all path parameters are required, however API Gateway will route
    # urls like /foo/{foo}/bar even if {foo} is missing (i.e. /foo//bar). In this case we
    # pass a null parameter value to the lambda and it does the check to see if all the
    # required parameters are present.
    
    # Strings in path need to be url decoded, then json encoded. The API Gateway provided 
    # $util.escapeJavaScript function encodes ' as \' which JSON does not allow, so these
    # are replaced with '. Note that with the Python ''' string format, all ' and " are 
    # literal and \\\\ becomes \\, which when passed to replaceAll inside a string, 
    # results in \'. Due to the way API Gateway's template processing works, despite the 
    # entire result being inside " the " characters in the call to replaceAll don't need 
    # to be escaped.

    not_a_dangling_comma = ',' if not last_parameter else ''
    name = parameter_object.get_string('name').value
    type = parameter_object.get_string('type').value

    if type == 'string':
        template = PATH_PARAMETER_REQUEST_TEMPLATE_MAPPING_STRING
    else:
        template = PATH_PARAMETR_REQUEST_TEMPLATE_MAPPING_NOT_STRING

    return template.format(name=name, comma=not_a_dangling_comma)


QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_STRING_WITH_DEFAULT = '''
#if($input.params().get('querystring').keySet().contains('{name}'))
    "{name}": "$util.escapeJavaScript($input.params().get('querystring').get('{name}')).replaceAll("\\\\'","'")"{comma}
#else
    "{name}": "{default}"{comma}
#end
'''

QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_STRING_WITHOUT_DEFAULT = '''
#if($input.params().get('querystring').keySet().contains('{name}'))
    "{name}": "$util.escapeJavaScript($input.params().get('querystring').get('{name}')).replaceAll("\\\\'","'")"{comma}
#else
    "{name}": null{comma}
#end
'''

QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_NOT_STRING_WITH_DEFAULT = '''
#if($input.params().get('querystring').keySet().contains('{name}'))
    "{name}": $input.params().get('querystring').get('{name}'){comma}
#else
    "{name}": {default}{comma}
#end
'''

QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_NOT_STRING_WITHOUT_DEFAULT = '''
#if($input.params().get('querystring').keySet().contains('{name}'))
    "{name}": $input.params().get('querystring').get('{name}'){comma}
#else
    "{name}": null{comma}
#end
'''

def _get_query_parameter_request_template_mapping(path, operation, parameter_object, last_parameter):

    # Swagger allows query parameters to specify required = false and allows an optional
    # default value to be provided. Reguardless of the required setting, we check to see
    # if a query parameter was provided and pass the default value, if one was provided,
    # or null to the lambda and it does the check to see if all the required parameters 
    # are present.

    # Strings in query are url decoded by API Gateway but they need to be json encoded. 
    # See note above. The difference here is that $util.urlDecode is not used.

    not_a_dangling_comma = ',' if not last_parameter else ''
    name = parameter_object.get_string('name').value
    default = parameter_object.get('default', default=None).value
    type = parameter_object.get_string('type').value

    if type == 'string':
        if default is not None:
            template = QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_STRING_WITH_DEFAULT
        else:
            template = QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_STRING_WITHOUT_DEFAULT
    else:
        if default is not None:
            template = QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_NOT_STRING_WITH_DEFAULT
        else:
            template = QUERY_PARAMETER_REQUEST_TEMPLATE_MAPPING_NOT_STRING_WITHOUT_DEFAULT

    return template.format(name=name, default=default, comma=not_a_dangling_comma)


def _has_parameters(parameters_object_stack):
    for entry in parameters_object_stack:
        if not entry.is_empty:
            return True
    return False



