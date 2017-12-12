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

import importlib
import inspect

from errors import ClientError

def api(f):
    '''Decorator that allows the dispatcher to call an handler function.'''
    f.is_dispatch_handler = True
    f.arg_spec = inspect.getargspec(f) # See __check_function_parameters below.
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

    print 'dispatching event', event
    
    module_name = event.get('module', None)
    if not module_name:
        raise ValueError('No "module" property was provided n the event object.')
        
    function_name = event.get('function', None)
    if not function_name:
        raise ValueError('No "function" property was provided in the event object.')

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

    request = Request(event, context)

    result = function(request, **parameters)

    print 'returning result', result

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
