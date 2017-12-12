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


def api(f):
    '''Decorator that allows the dispatcher to call an handler function.'''
    f.is_dispatch_handler = True
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

    module = importlib.import_module('plugin.CloudGemFramework.api.' + module_name)

    function = getattr(module, function_name, None)

    if not function:
        raise ValueError('The module "{}" does not have an "{}" attribute.'.format(module_name, function_name))

    if not callable(function):
        raise ValueError('The module "{}" attribute "{}" is not a function.'.format(module_name, function_name))

    if not hasattr(function, 'is_dispatch_handler'):
        raise ValueError('The module "{}" function "{}" is not a service dispatch handler.'.format(module_name, function_name))

    parameters = event.get('parameters', {})

    request = Request(event, context)

    result = function(request, **parameters)

    print 'returning result', result

    return result
 
