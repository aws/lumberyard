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

from resource_manager_common import module_utils

import resource_management
import swagger_processor
import cloud_gem_portal

def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    subparser = subparsers.add_parser("cloud-gem-framework", help="Commands to manage CloudGems and CloudGem Portal")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    cgf_subparsers = subparser.add_subparsers(dest = 'subparser_name', metavar='COMMAND')

    resource_management.add_cli_commands(cgf_subparsers, add_common_args)
    swagger_processor.add_cli_commands(cgf_subparsers, add_common_args)
    cloud_gem_portal.add_cli_commands(cgf_subparsers, add_common_args)

    subparser = cgf_subparsers.add_parser('generate-service-api-code', help='Generate component and lambda function code to support a service API described by a swagger.json file provided by resource group.')
    subparser.add_argument('--gem', '-g', '--resource-group', '-r', required=True, metavar='GEM', help='The name of the gem that provides a swagger.json file.')
    subparser.add_argument('--update-waf-files', required=False, action='store_true', help='Causes the generated files to be added to the gem\'s .waf_files file.')
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

    # The gem argument can be the full path to a gem directory (with a gem.json file 
    # in it), the name of a gem enabled for the project, or the name of a project
    # local resource group (support for which is deprecated). The gem full path option 
    # is used when creating a gem because this command is executed as a seperate
    # process. That happens so that (a) the user can see what command they can
    # use to re-generate the code after making swagger changes, and (b) because 
    # the core lmbr_aws stuff doesn't have a good way to call this code directly.
    # TODO: finish CGF/Core refactoring....

    gem = context.gem.get_by_root_directory_path(args.gem)
    if gem is None:
        gem = context.gem.get_by_name(args.gem)
    if gem is None:
        # TODO: remove legacy support for project local resource groups. Deprecated
        # in Lumberyard 1.11 (CGF 1.1.1).
        resource_group = context.resource_groups.get(args.gem) # will raise if doesn't exist
        swagger_file_path = os.path.join(resource_group.directory_path, 'swagger.json')
    else:
        swagger_file_path = os.path.join(gem.aws_directory_path, 'swagger.json')

    jinja = __initialize_jinja(context)

    swagger = __load_swagger(context, swagger_file_path)

    destination_code_path = args.component_client_path
    if destination_code_path is None:
        if gem:
            base_code_path = gem.cpp_base_directory_path
            destination_code_path = gem.cpp_aws_directory_path
        else:
            base_code_path = resource_group.cpp_base_directory_path
            destination_code_path = gem.cpp_aws_directory_path
        destination_code_path = os.path.join(destination_code_path, 'ServiceApi')
    else:
        base_code_path = destination_code_path

    if gem:
        namespace_name = gem.name
    else:
        namespace_name = resource_group.name

    waf_files_updates = __generate_component_client(context, base_code_path, destination_code_path, namespace_name, jinja, swagger, gem)

    waf_files_updated = False
    if args.update_waf_files and gem:
        waf_files_updated = __update_gem_waf_files(context, gem, waf_files_updates)

    if not waf_files_updated:
        print '\nNOTICE: Add the created files to the approriate .waf_files file to include them in your project builds.\n'

def __update_gem_waf_files(context, gem, waf_files_updates):

    waf_files_file_path = os.path.join(gem.root_directory_path, 'Code', gem.name.lower() + '.waf_files')
    if not os.path.exists(waf_files_file_path):
        print '\nWARNING: could not find gem\'s .waf_files file at {}.'.format(waf_files_file_path)
        return False

    with open(waf_files_file_path, 'r') as file:
        waf_files_content = json.load(file)

    __merge_waf_file_updates(waf_files_content, waf_files_updates)

    context.view.saving_file(waf_files_file_path)
    with open(waf_files_file_path, 'w') as file:
        json.dump(waf_files_content, file, indent=4, sort_keys=True)

    return True


def __merge_waf_file_updates(dst, src):
    
    for src_key, src_value in src.iteritems():
        dst_value = dst.get(src_key, None)
        if dst_value is None:
            dst[src_key] = src_value
        elif isinstance(dst_value, list):
            if not isinstance(src_value, list):
                raise RuntimeError('Expected source value type list on key {} when merging {} into {}.'.format(src_key, src, dst))
            for entry in src_value:
                if entry not in dst_value:
                    dst_value.append(entry)
        elif isinstance(dst_value, dict):
            if not isinstance(src_value, dict):
                raise RuntimeError('Expected source value type dict on key {} when merging {} into {}.'.format(src_key, src, dst))
            __merge_waf_file_updates(dst_value, src_value)
        else:
            raise RuntimeError('Exepected destination value type list or dict on key {} when merging {} into {}.'.format(src_key, src, dst))


def __initialize_jinja(context):
    third_party_json_path = os.path.join(context.config.root_directory_path, "BinTemp", "3rdParty.json")

    if not os.path.isfile(third_party_json_path):
        raise HandledError("The file 3rdParty.json was not found at {}. You must run lmbr_waf configure before you can generate service API code.".format(os.path.dirname(third_party_json_path)))

    with open(third_party_json_path, "r") as fp:
        third_party_json = json.load(fp)

    third_party_root = third_party_json['3rdPartyRoot']
    sdks = third_party_json['SDKs']
    sdk_paths = {}

    for sdk_name in ('jinja2', 'markupsafe'):
        sdk_info = sdks.get(sdk_name, None)

        if not sdk_info:
            raise HandledError('The {} Python library was not found at {}. You must select the "Compile the game code" option in SetupAssistant before you can generate service API code.'.format(sdk_name, third_party_root))

        sdk_path = os.path.join(third_party_root, sdk_info['base_source'], 'x64')
        sdk_paths[sdk_name] = sdk_path
        sys.path.append(sdk_path)

    jinja_path = os.path.join(sdk_paths['jinja2'], 'jinja2')
    loaders_module = module_utils.load_module('loaders', jinja_path)
    template_path = os.path.join(os.path.dirname(__file__), 'templates')
    print 'template_path', template_path
    loader = loaders_module.FileSystemLoader(template_path)

    environment_module = module_utils.load_module('environment', jinja_path)
    environment = environment_module.Environment(loader=loader)

    return environment


def __load_swagger(context, swagger_file_path):

    if not os.path.isfile(swagger_file_path):
        raise HandledError('The {} resource group does not provide a swagger.json file ({} does not exist).'.format(args.resource_group, swagger_file_path))

    with open(swagger_file_path, 'r') as file:
        return json.load(file)


def __write_file(template, jinja_json, out_file):
    result = template.render(json_object = jinja_json)

    print 'Writing file {}.'.format(out_file)
    with open(out_file, 'w') as file:
        file.write(result)


def __generate_component_client(context, base_code_path, destination_code_path, namespace_name, jinja, swagger, gem):

    if not os.path.exists(destination_code_path):
        print 'Making directory {}'.format(destination_code_path)
        os.makedirs(destination_code_path)

    template_h = jinja.get_template('component-client/component_template.h')
    out_path_h = os.path.join(destination_code_path, '{}ClientComponent.h'.format(namespace_name))

    template_cpp = jinja.get_template('component-client/component_template.cpp')
    out_path_cpp = os.path.join(destination_code_path, '{}ClientComponent.cpp'.format(namespace_name))

    jinja_json = component_gen_utils.generate_component_json(namespace_name, swagger)
    jinja_json["UUIDs"] = component_gen_utils.get_UUIDs(out_path_cpp, jinja_json)

    jinja_json["HasStdAfx"] = component_gen_utils.has_stdafx_files(gem.cpp_base_directory_path)

    __write_file(template_h, jinja_json, out_path_h)
    __write_file(template_cpp, jinja_json, out_path_cpp)

    return {
        'auto': {
            'Include': [ __make_wscript_relative_path(base_code_path, out_path_h) ],
            'Source': [ __make_wscript_relative_path(base_code_path, out_path_cpp) ]
        }
    }


def __make_wscript_relative_path(base_path, file_path):
    return os.path.relpath(file_path, base_path).replace(os.sep, '/')


def add_cli_view_commands(hook, view_context, **kwargs):
    def create_admin(self, administrator_name, password, message):
        self._output_message(message + '\n\tUsername: {}\n\tPassword: {}'.format(administrator_name, password))
    view_context.create_admin = types.MethodType(create_admin, view_context)

def add_gui_view_commands(hook, view_context, **kwargs):
    def create_admin(self, administrator_name, password, message):
        self._output('create-admin', {'administrator_name' : administrator_name, 'password': password, 'message': message})
    view_context.create_admin = types.MethodType(create_admin, view_context)