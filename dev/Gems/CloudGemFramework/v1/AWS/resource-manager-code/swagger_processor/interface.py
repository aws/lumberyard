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
import re

from resource_manager.errors import HandledError

from resource_manager_common import service_interface

import swagger_processor
import swagger_json_navigator

import lambda_dispatch


def process_interface_implementation_objects(context, swagger_navigator):
    for path, path_object in swagger_navigator.get_object('paths').items():
        interface_implementation_object = path_object.get_object(service_interface.INTERFACE_IMPLEMENTATION_OBJECT_NAME, default = None)
        if interface_implementation_object.is_object:
            process_interface_implementation_object(context, interface_implementation_object)
            interface_implementation_object.parent.remove(interface_implementation_object.selector)


def process_interface_implementation_object(context, interface_implementation_object):

    external_parameters = interface_implementation_object.parent.get_array_value('parameters', default = [])
    for external_parameter in external_parameters:
        if service_interface.INTERFACE_PARAMETER_OBJECT_NAME not in external_parameter.keys():
            raise RuntimeError(
                'Implementing an interface on {}, a path with parameters, is an advanced feature. To enable it, use "{}: True" in the parameter definition. Interfaces with external parameters are not automatically published in the directory service.'.format(
                    interface_implementation_object.parent.selector,
                    service_interface.INTERFACE_PARAMETER_OBJECT_NAME
                )
            )

    interface_list = interface_implementation_object.get_array('interface', auto_promote_singletons = True)
    for index, interface in interface_list.items():

        if interface.is_object:
            interface_id = interface.get_string('id').value
            lambda_dispatch_for_paths = interface.get_object('lambda_dispatch', default = {}).get_object('paths', default = {}).value
        else:
            interface_id = interface.value
            lambda_dispatch_for_paths = {}

        interface_swagger = load_interface_swagger(context, interface_id)

        mappings = insert_interface_definitions(interface_implementation_object.root, interface_swagger, interface_id, external_parameters) 
        insert_interface_paths(interface_implementation_object.parent, interface_swagger, interface_id, mappings, lambda_dispatch_for_paths, external_parameters)

        if not external_parameters:
            add_interface_base_path_metadata(interface_implementation_object.root, interface_id, interface_implementation_object.parent.selector)


def insert_interface_definitions(target_swagger, source_swagger, interface_id, external_parameter):

    mappings = {}
    
    source_definitions = source_swagger.get_object('definitions', default = None)
    if not source_definitions.is_none:

        target_definitions = target_swagger.get_or_add_object('definitions')

        # First determine the mapping from the interface name to the target name for all definitions.
        for source_name, source_definition in source_definitions.items():

            target_name = source_name

            # If there is already a definition with the same name, and they are identical,
            # them we can resue it. Otherwise, we need to generate an unique name for the
            # source definition.
            target_definition = target_definitions.get_object(target_name, default = None)
            if not target_definition.is_none and target_definition.value != source_definition.value:
                target_name = increment_trailing_number(target_name)
                while target_definitions.contains(target_name):
                    target_name = increment_trailing_number(target_name)

            mappings[source_name] = target_name

        # Now apply those mappings to any references nested in the definitions.
        for source_name, source_definition in source_definitions.items():

            target_name = mappings[source_name]

            # If the definition already exists, it is because we are reusing an existing 
            # definition as determined in the loop above. In this case, there is no need
            # to copy the source definition.
            if not target_definitions.contains(target_name):
                target_definition_object_value = copy_with_updated_refs(source_definition, mappings)
                target_definitions.add_object(target_name, target_definition_object_value)

            if not external_parameter:
                add_interface_definition_metadata(target_swagger, interface_id, target_name)

    return mappings


def increment_trailing_number(name):
    match = re.match('(.*?)([0-9]+)$', name)
    if match:
        base = match.group(1)
        number = int(match.group(2)) + 1
    else:
        base = name
        number = 1
    return base + str(number)


def insert_interface_paths(parent_path_object, interface_swagger, interface_id, mappings, lambda_dispatch_for_paths, external_parameters):
    for interface_path, interface_path_object in interface_swagger.get_object('paths').items():

        # Swagger requires that paths start with an /, so no seperator needed. The
        # root path, '/', is handled specially to avoid duplicate leading /.
        if parent_path_object.selector == '/':
            path = interface_path 
        else:
            path = parent_path_object.selector + interface_path

        if parent_path_object.parent.contains(path):
            raise RuntimeError('Cannot insert paths for interface {}. A {} path already exists in the swagger.'.format(
                interface_id, path))
        
        path_object_value = copy_with_updated_refs(interface_path_object, mappings)

        if external_parameters:
            parameters = path_object_value.setdefault('parameters', [])
            parameters.extend(external_parameters)

        lambda_dispatch_for_path = lambda_dispatch_for_paths.get(interface_path, {})
        for operation_name, operation_value in path_object_value.iteritems():
            lambda_dispatch_for_operation = lambda_dispatch_for_path.get(operation_name, None)
            if lambda_dispatch_for_operation:
                operation_value[lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = lambda_dispatch_for_operation

        parent_path_object.parent.add_object(path, path_object_value)

        if not external_parameters:
            add_interface_path_metadata(parent_path_object.root, interface_id, path)



def add_interface_base_path_metadata(target_swagger, interface_id, base_path):
    interface_metadata = get_interface_metadata(target_swagger, interface_id)
    interface_metadata.value['basePath'] = base_path


def add_interface_path_metadata(target_swagger, interface_id, path):
    interface_metadata = get_interface_metadata(target_swagger, interface_id)
    interface_metadata.get_or_add_array('paths').value.append(path)


def add_interface_definition_metadata(target_swagger, interface_id, name):
    interface_metadata = get_interface_metadata(target_swagger, interface_id)
    interface_metadata.get_or_add_array('definitions').value.append(name)


def get_interface_metadata(target_swagger, interface_id):
    metadata_object = target_swagger.get_or_add_object(service_interface.INTERFACE_METADATA_OBJECT_NAME)
    return metadata_object.get_or_add_object(interface_id)


def copy_with_updated_refs(source_navigator, mappings):

    if source_navigator.is_ref:
        result = update_ref(source_navigator, mappings)
    elif source_navigator.is_object:
        result = {}
        for selector, child_navigator in source_navigator.items():
            result[selector] = copy_with_updated_refs(child_navigator, mappings)
    elif source_navigator.is_array:
        result = []
        for selector, child_navigator in source_navigator.items():
            result.append(copy_with_updated_refs(child_navigator, mappings))
    else:
        result = source_navigator.value

    return result


def update_ref(source_navigator, mappings):
    source_ref = source_navigator.get_string('$ref')
    if not source_ref.value.startswith('#'):
        raise RuntimeError('Only local definition references can be used in interface definitions. Found {} in {}.'.format(source_navigator.value, interface_id))
    ref_parts = source_ref.value.split('/')
    ref_parts[2] = mappings[ref_parts[2]]
    target_ref = '/'.join(ref_parts)
    return { '$ref': target_ref }


def load_interface_swagger(context, interface_id):
    
    gem_name, interface_name, interface_version = parse_interface_id(interface_id)
    
    interface_file_name = interface_name + '_' + str(interface_version).replace('.', '_') + '.json'

    gem = context.gem.get_by_name(gem_name)
    if gem is None:
        raise HandledError('The gem {} does not exist or is not enabled for the project.'.format(gem_name))

    interface_file_path = os.path.join(gem.aws_directory_path, 'api-definition', interface_file_name)
    if not os.path.exists(interface_file_path):
        raise HandledError('Could not find the definition for interface {} at {}.'.format(interface_id, interface_file_path))

    try:
        with open(interface_file_path, 'r') as file:
            interface_swagger = json.load(file)
            swagger_processor.validate_swagger(interface_swagger)
    except Exception as e:
        raise HandledError('Cloud not load the definition for interface {} from {}: {}'.format(interface_id, interface_file_path, e.message))

    return swagger_json_navigator.SwaggerNavigator(interface_swagger)



def parse_interface_id(interface_id):
    try:
        return service_interface.parse_interface_id(interface_id)
    except Exception as e:
        raise HandledError(e.message)