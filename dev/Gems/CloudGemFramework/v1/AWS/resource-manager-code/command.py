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

import json
import os
import sys
import imp
import string
import types

import component_gen_utils

from resource_manager.errors import HandledError
import resource_manager.cli

import resource_management
import swagger_processor
import cloud_gem_portal

def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    subparser = subparsers.add_parser("cloud-gem-framework", help="Commands to manage CloudGems and CloudGem Portal")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    cgf_subparsers = subparser.add_subparsers(dest = 'subparser_name')

    resource_management.add_cli_commands(cgf_subparsers, add_common_args)
    swagger_processor.add_cli_commands(cgf_subparsers, add_common_args)
    cloud_gem_portal.add_cli_commands(cgf_subparsers, add_common_args)

    subparser = cgf_subparsers.add_parser('generate-service-api-code', help='Generate component and lambda function code to support a service API described by a swagger.json file provided by resource group.')
    subparser.add_argument('--resource-group', required=True, metavar='GROUP', help='The name of a resource group that provides a swagger.json file.')
    subparser.add_argument('--component-client-path', required=False, default=None,
                           help='The path where the component client code will be written. Defaults to the '
                                'Code\{game}\AWS\{group}\ServiceAPI directory, or the Gem\Code\AWS\ServiceAPI '
                                'directory if the resource group is defined by a Gem.')
    add_common_args(subparser)
    subparser.set_defaults(func=__generate_service_api_code)


def add_gui_commands(hook, handlers, **kwargs):
    handlers['cloud-gem-portal'] = cloud_gem_portal.open_portal
    handlers['add-service-api-resources'] = resource_management.add_resources

def __generate_service_api_code(context, args):

    if args.resource_group not in context.resource_groups:
        raise HandledError('The resource group {} does not exist.'.format(args.resource_group))

    jinja = __initialize_jinja(context, args)
    swagger = __load_swagger(context, args)

    __generate_component_client(context, args, jinja, swagger)



def __initialize_jinja(context, args):

    jinja_path = os.path.join(context.config.root_directory_path, 'Code', 'SDKs', 'jinja2', 'x64')
    if not os.path.isdir(jinja_path):
        raise HandledError('The jinja2 Python library was not found at {}. You must select the "Compile the game code" option in SetupAssistant before you can generate service API code.'.format(jinja_path))
    sys.path.append(jinja_path)

    markupsafe_path = os.path.join(context.config.root_directory_path, 'Code', 'SDKs', 'markupsafe', 'x64')
    if not os.path.isdir(markupsafe_path):
        raise HandledError('The markupsafe Python library was not found at {}. You must select the "Complile the game code" option in SetupAssistant before you can generate service API code.'.format(markupsafe_path))

    sys.path.append(markupsafe_path)

    loaders_module = __load_module('loaders', os.path.join(jinja_path, 'jinja2'))
    template_path = os.path.join(os.path.dirname(__file__), 'templates')
    print 'template_path', template_path
    loader = loaders_module.FileSystemLoader(template_path)

    environment_module = __load_module('environment', os.path.join(jinja_path, 'jinja2'))
    environment = environment_module.Environment(loader=loader)

    return environment


def __load_module(name, path):

    path = [ path ]

    fp, pathname, description = imp.find_module(name, path)

    try:
        module = imp.load_module(name, fp, pathname, description)
        return module
    finally:
        if fp:
            fp.close()


def __load_swagger(context, args):
    resource_group = context.resource_groups.get(args.resource_group)

    resource_group_path = resource_group.directory_path
    swagger_file_path = os.path.join(resource_group_path, 'swagger.json')

    if not os.path.isfile(swagger_file_path):
        raise HandledError('The {} resource group does not provide a swagger.json file ({} does not exist).'.format(args.resource_group, swagger_file_path))

    with open(swagger_file_path, 'r') as file:
        return json.load(file)


def __generate_component_client(context, args, jinja, swagger):

    destination_code_path = args.component_client_path
    if destination_code_path is None:
        resource_group = context.resource_groups.get(args.resource_group, None)
        if resource_group.is_gem:
            destination_code_path = os.path.abspath(os.path.join(resource_group.directory_path, '..', 'Code', 'Include', 'AWS', 'ServiceAPI'))
        else:
            destination_code_path = os.path.join(resource_group.game_cpp_code_path, 'ServiceAPI')

    if not os.path.exists(destination_code_path):
        print 'Making directory {}'.format(destination_code_path)
        os.makedirs(destination_code_path)

    template = jinja.get_template('component-client/component_template.h')

    out_file_path = os.path.join(destination_code_path, '{}ClientComponent.h'.format(args.resource_group))

    jinja_json = component_gen_utils.generate_component_json(args.resource_group, swagger)
    jinja_json["UUIDs"] = component_gen_utils.get_UUIDs(out_file_path, jinja_json)

    result = template.render(json_object = jinja_json)

    print 'Writing file {}.'.format(out_file_path)
    print 'Add to the approriate .waf_files file to include in your project'
    with open(out_file_path, 'w') as file:
        file.write(result)

    lambda_code_path = context.resource_groups.get(args.resource_group, None).lambda_code_path
    print 'lambda_code_path', lambda_code_path

def add_cli_view_commands(hook, view_context, **kwargs):
    def create_admin(self, administrator_name, password, message):
        self._output_message(message + '\n\tUsername: {}\n\tPassword: {}'.format(administrator_name, password))
    view_context.create_admin = types.MethodType(create_admin, view_context)

def add_gui_view_commands(hook, view_context, **kwargs):
    def create_admin(self, administrator_name, password, message):
        self._output('create-admin', {'administrator_name' : administrator_name, 'password': password, 'message': message})
    view_context.create_admin = types.MethodType(create_admin, view_context)