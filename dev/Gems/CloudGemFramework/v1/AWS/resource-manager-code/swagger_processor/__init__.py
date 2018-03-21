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

import os
import json
import copy

from swagger_json_navigator import SwaggerNavigator
from resource_manager.errors import HandledError

import swagger_spec_validator

import interface
import lambda_dispatch


def add_cli_commands(subparsers, addCommonArgs):

    # add-service-api-resources
    subparser = subparsers.add_parser('service-api-process-swagger', help='Process the Cloud Canvas defined extension objects in a swagger.json to produce swagger definitions that can be imported into AWS API Gateway. This process is performed automatically before uploading a resource group''s swagger.json file.')
    group = subparser.add_mutually_exclusive_group(required=True)
    group.add_argument('--resource-group', '-r', metavar='GROUP', help='The name of the resource group.')
    group.add_argument('--input', '-i', metavar='FILE-PATH', help='The file from which the swagger will be read.')
    subparser.add_argument('--output', '-o', required=False, metavar='FILE-PATH', help='The file to which the processed swagger will be written. By default the output is written to stdout.')
    addCommonArgs(subparser)
    subparser.set_defaults(func=_process_swagger_cli)


def _process_swagger_cli(context, args):
    
    if args.resource_group:
        resource_group = context.resource_groups.get(args.resource_group)
        swagger_path = os.path.join(resource_group.directory_path, 'swagger.json')
        if not os.path.isfile(swagger_path):
            raise HandledError('The resource group {} has no swagger file.'.format(args.resource_group))
    else:
        swagger_path = args.input
        if not os.path.isfile(swagger_path):
            raise HandledError('The swagger file {} does not exist.'.format(swagger_path))

    result = process_swagger_path(context, swagger_path)

    if args.output:
        with open(args.output, 'w') as file:
            file.write(result)
    else:
        print result


def process_swagger_path(context, swagger_path):

    try:
        with open(swagger_path, 'r') as swagger_file:
            swagger_content = swagger_file.read()
    except IOError as e:
        raise HandledError('Could not read file {}: {}'.format(swagger_path, e.message))

    try:
        swagger = json.loads(swagger_content)
    except ValueError as e:
        raise HandledError('Cloud not parse {} as JSON: {}'.format(swagger_path, e.message))

    try:
        process_swagger(context, swagger)
    except ValueError as e:
        raise HandledError('Could not process {}: {}'.format(swagger_path, e.message))

    try:
        content = json.dumps(swagger, sort_keys=True, indent=4)
    except ValueError as e:
        raise HandledError('Could not convert processed swagger to JSON: {}'.format(e.message))

    return content

def process_swagger(context, swagger):
    
    # make sure we are starting with a valid document
    validate_swagger(swagger)

    # process the swagger, order matters
    swagger_navigator = SwaggerNavigator(swagger)
    interface.process_interface_implementation_objects(context, swagger_navigator) 
    lambda_dispatch.process_lambda_dispatch_objects(context, swagger_navigator)

    # make sure we produce a valid document
    validate_swagger(swagger)

def validate_swagger(swagger):
    try:
        # The validator inserts x-scope objects into the swagger when validating
        # references. We validate against a copy of the swagger to keep these from
        # showing up in our output.

        # Hitting errors in the definitions dictionary gives better error messages,
        # so run the validator with no paths first
        swagger_definitions_only = copy.deepcopy(swagger)
        swagger_definitions_only['paths'] = {}
        swagger_spec_validator.validator20.validate_spec(swagger_definitions_only)

        swagger_copy = copy.deepcopy(swagger)
        swagger_spec_validator.validator20.validate_spec(swagger_copy)

    except swagger_spec_validator.SwaggerValidationError as e:
        try:
            content = json.dumps(swagger, indent=4, sort_keys=True)
        except:
            content = swagger
        raise ValueError('Swagger validation error: {}\n\nSwagger content:\n{}\n'.format(e.message, content))
