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

from __future__ import print_function

from copy import deepcopy
import importlib
import inspect
from types import DictionaryType, FunctionType, ListType

from error import ClientError

def api(function_to_decorate=None, unlogged_parameters=None, logging_filter=None):
    '''Decorator that allows the dispatcher to call an handler function.'''

    # If no arguments are provided to @api, this function acts as a normal decorator and
    # receives the function to decorate as a single positional argument.
    if function_to_decorate:
        return __decorate_api(function_to_decorate, unlogged_parameters, logging_filter)

    # If arguments are provided to @api(...), this function acts as a decorator factory.
    # The function to decorate will be passed as a single positional argument to the following lambda.
    return lambda f: __decorate_api(f, unlogged_parameters, logging_filter)

def __decorate_api(f, unlogged_parameters, logging_filter):
    f.is_dispatch_handler = True
    f.arg_spec = inspect.getargspec(f) # See __check_function_parameters below.
    f.unlogged_parameters = unlogged_parameters
    f.logging_filter = logging_filter
    return f

class Request(object):
    '''Enecapuslates the event and context object's passed to the 
    handler function by AWS Lambda.'''

    def __init__(self, event, context):
        self.__context = context
        self.__event = event

    @property
    def context(self):
        return self.__context

    @property
    def event(self):
        return self.__event


def dispatch(event, context):
    '''Main entry point for the service AWS Lambda Function. It dispatches requests 
    to the correct code for processing.

    Arguments:

        event: An object containing the request data as constructed by the service's
        AWS API Gateway. This object is expected to include the following properties:

            * module - the name of the module that implements the handler function.
              This name is prefixed with "api." so that only modules defined in the
              api directory will be loaded.

            * function - the name of the handler function. The function must be 
              decorated using @dispatch.handler or the dispatcher will not execute it.
              The function will be passed a dispatch.Reqeust object followed by the
              value of the parameters dict passed as **kwargs.

            * parameters - an dict containing parameter values.

        context: The AWS Lambda defined context object for the request. See 
        https://docs.aws.amazon.com/lambda/latest/dg/python-context-object.html.

    '''

    module_name = event.get('module', None)
    if not module_name:
        raise ValueError('No "module" property was provided n the event object.')
        
    function_name = event.get('function', None)
    if not function_name:
        raise ValueError('No "function" property was provided in the event object.')

    print('request id {} has module {} and function {}'.format(context.aws_request_id, module_name, function_name))

    module = importlib.import_module('api.' + module_name)

    function = getattr(module, function_name, None)

    if not function:
        raise ValueError('The module "{}" does not have an "{}" attribute.'.format(module_name, function_name))

    if not callable(function):
        raise ValueError('The module "{}" attribute "{}" is not a function.'.format(module_name, function_name))

    if not hasattr(function, 'is_dispatch_handler'):
        raise ValueError('The module "{}" function "{}" is not a service dispatch handler.'.format(module_name, function_name))

    parameters = event.get('parameters', {})

    __check_function_parameters(function, parameters)

    # The request parameters are logged after finding the handler function since that's where the logging filters are defined.
    # Request logging is also after the request validation so that the logging filters don't have to know how to filter unexpected data.
    filtered_parameters = __apply_logging_filter(function, parameters)
    print('dispatching request id {} function {} with parameters {}'.format(context.aws_request_id, 'api.{}.{}'.format(module_name, function_name), filtered_parameters))

    request = Request(event, context)

    result = function(request, **parameters)

    filtered_result = __apply_logging_filter(function, result)
    print('returning result {}'.format(filtered_result))

    return result


def __check_function_parameters(function, parameters):

    # arg_spec is a named tuple as produced by inspect.getargspec: ArgSpec(args, varargs, keywords, defaults)
    arg_spec = function.arg_spec

    # If the function has a **kwargs parameter, it will soak up all values that don't match parameter names.
    has_kwargs_parameter = arg_spec.keywords is not None

    # Check to see if all expected parameters have a value provided and that there aren't any values for 
    # which there are no parameters.
    expected_parameters = set(arg_spec.args)
    unexpected_paramters = set()
    for parameter_name, parameter_value in parameters.iteritems():
        if parameter_name in expected_parameters:
            if parameter_value is not None:
                if parameter_name == arg_spec.args[0]:
                    raise ValueError('Invalid handler arguments. The first parameter\'s name, {}, matches an api parameter name. Use an unique name for the first parameter, which is always a service.Request object.'.format(parameter_name))
                expected_parameters.remove(parameter_name)
        elif not has_kwargs_parameter: # There are no "unexpected" parameters if there is a **kwargs parameter.
            unexpected_paramters.add(parameter_name)
    
    # The request object is passed as the first parameter, reguardless of it's name.
    expected_parameters.remove(arg_spec.args[0])

    # Values do not need to be provided for any parameter with a default value. The arg_spec.defaults
    # array contains default values for the last len(arg_spec.defaults) arguments in arg_spec.args.
    if arg_spec.defaults:
        args_index = len(arg_spec.args) - 1
        default_index = len(arg_spec.defaults) - 1
        while default_index >= 0:
            arg_name = arg_spec.args[args_index]
            if arg_name in expected_parameters:
                expected_parameters.remove(arg_name)
            default_index -= 1
            args_index -= 1

    # If there are any expected parameters for which values are not present, or parameters with names
    # that are not expected, generate a 400 response for the client.
    if expected_parameters or unexpected_paramters:
        error_message = ''
        if expected_parameters:
            if error_message: error_message += ' '
            error_message += 'Expected the following parameters: {}.'.format(', '.join(expected_parameters))
        if unexpected_paramters:
            if error_message: error_message += ' '
            error_message += 'The following parameters are unexpected: {}.'.format(', '.join(unexpected_paramters))
        error_message += ' Check the documentation for the the API your calling.'
        raise ClientError(error_message)

def __apply_logging_filter(function, parameters):
    if not function.unlogged_parameters and not function.logging_filter:
        return parameters

    result = deepcopy(parameters)
    if function.unlogged_parameters:
        for name in function.unlogged_parameters:
            if name in result and result[name] is not None and result[name] != '':
                result[name] = '<redacted>'

    if function.logging_filter:
        function.logging_filter(result)

    return result
