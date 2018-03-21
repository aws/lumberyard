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
# $Revision: #3 $

from cgf_utils import properties
from cgf_utils import custom_resource_response
from cgf_utils import aws_utils

from cgf_utils.properties import ValidationError

def handler(event, context):
    
    props = properties.load(event, {
        'Input': properties.Dictionary(),
    })

    stack_name = aws_utils.get_stack_name_from_stack_arn(event['StackId'])
    physical_resource_id = stack_name + '-' + event['LogicalResourceId']

    output = _process_dict(props.Input)

    return custom_resource_response.success_response(output, physical_resource_id)


def _process_dict(input):

    output = {}

    for key, value in input.iteritems():
        output[key] = _process_value(value)

    return output


def _process_value(input):

    if _is_function(input):
        return _process_function(input)

    if isinstance(input, dict):
        return _process_dict(input)

    if isinstance(input, list):
        return _process_list(input)

    return input


def _process_list(input):

    output = []

    for value in input:
        output.append(_process_value(value))

    return output


def _is_function(value):
    
    if not isinstance(value, dict):
        return False

    for key in value.keys():
        if key.startswith('HelperFn::'):
            if len(value) == 1:
                return True
            else:
                raise ValidationError('{} must be the only property in an object.'.format(key))

    return False


def _process_function(input):

    [(key, value)] = input.items()

    handler = _function_handlers.get(key, None)
    if handler is None:
        raise ValidationError('Unsupported function: {}'.format(key))

    return handler(value)


def _lower_case(input):

    if not isinstance(input, basestring):
        raise ValidationError('HelperFn::LowerCase value must be a string: {}'.format(input))

    return input.lower()



_function_handlers = {
    'HelperFn::LowerCase': _lower_case
}
