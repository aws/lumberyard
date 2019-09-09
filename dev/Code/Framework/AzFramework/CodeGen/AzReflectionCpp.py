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
import re
from collections import defaultdict
from az_code_gen.base import *
from az_code_gen.clang_cpp import format_cpp_annotations, resolve_annotation, store_annotation

def quote(s):
    return '"' + s + '"'

def format_description(annotations, scope, name):
    # description has 2 args, name and desc, store them into Name and Description separately
    resolved_name = resolve_annotation(annotations, scope + '::Name', quote(name))
    description = resolve_annotation(annotations, scope + '::Description', [resolved_name, quote('No Description')])
    if isinstance(description, basestring):
        description = [resolved_name, description]
    if resolved_name != quote(name): # if there was a specified name, that should win
        description[0] = resolved_name
    store_annotation(annotations, scope + '::Name', description[0])
    store_annotation(annotations, scope + '::Description', description[1])


def configure_reflection(object):
    if object.get('type') in ('class', 'struct'):
        # look for there to either be a Serialize dict or just AzClass::Serialize(true/false)
        object['serialize'] = resolve_annotation(object['annotations'], 'AzClass::Serialize', False) != False
        # The field serialization default is if serialize is true and not set to opt-in
        field_serialize_default = object['serialize'] in (True, None, "true") and not resolve_annotation(object['annotations'], 'AzClass::Serialize::OptIn', False) != False

        object['reflectForEdit'] = resolve_annotation(object['annotations'], 'AzClass::Edit', False) != False
        # Behavior context class tags
        object['behavior'] = False
        behavior_class_tags = ('BehaviorName',)
        for tag in behavior_class_tags:
            if resolve_annotation(object['annotations'], 'AzClass::' + tag, False) != False:
                object['behavior'] = True

        # Method tags checks
        for method in object['methods']:
            # Behavior context function tags
            behavior_method_tags = ('Behavior', 'BehaviorReadOnly')
            for tag in behavior_method_tags:
                if resolve_annotation(method['annotations'], 'AzFunction::' + tag, False) != False:
                    object['behavior'] = True

        # Fields tags checks
        serialized_fields = 0

        for field in object['fields']:
            annotations = field['annotations']
            annotations.setdefault('AzField', {}) # force AzField to exist, even if it's empty
            # NOTE: TODO: We should exclude special fields in the actual script however AzProperty seem to NOT be used, so this is temporary till add CG support
            if 'm_azCodeGenInternal' in field['name']:
                field['serialize'] = False
            else:
                field['serialize'] = resolve_annotation(annotations, 'AzField::Serialize', field_serialize_default) in (True, None, "true")
                if field['serialize']:
                    serialized_fields += 1

            # Look for edit property attributes and mark the class for editing/serialization
            field['edit'] = False
            tags_that_imply_edit = ('Edit', 'ElementUIHandler', 'UIHandler', 'Group', 'Name', 'Description')
            for tag in tags_that_imply_edit:
                if resolve_annotation(annotations, 'AzField::' + tag):
                    object['reflectForEdit'] = True
                    field['edit'] = True
                    # Fields not marked for Serialize need to be fixed up. Edit requires Serialize
                    if not field['serialize']:
                        OutputError('{}({}): Error: Field {} marked with tags used for Edit but not Serialize. Serialize is required for Edit features. Implicitly marking field for Serialize.'.format(field['file_name'], field['line_number'], field['qualified_name']))
                        field['serialize'] = True # edit implies serialize
                        serialized_fields += 1

            # BehaviorContext field tags
            behavior_field_tags = ('Behavior', 'BehaviorReadOnly')
            for tag in behavior_field_tags:
                if resolve_annotation(annotations, 'AzField::' + tag, False) != False:
                    object['behavior'] = True

        # Ensure that the object is marked for serialization if fields need it
        object['serialize'] = object['serialize'] or serialized_fields > 0
    elif object.get('type') in ('enum'):
        object['serialize'] = resolve_annotation(object['annotations'], 'AzEnum') is not None
        object['reflectForEdit'] = resolve_annotation(object['annotations'], 'AzEnum::Values') is not None
        if resolve_annotation(object['annotations'], 'AzEnum::Behavior', False) != False:
            object['behavior'] = True
    else:
        OutputPrint('configure_reflection: Unexpected object: {} {}'.format(object.get('type'), object.get('qualified_name')))
    # determine of the object needs Reflection function.
    if object['serialize'] or object['behavior'] or resolve_annotation(object['annotations'], 'AzClass::CustomReflection', False):
        object['hasReflection'] = True
    else:
        object['hasReflection'] = False

def sort_fields_by_group(fields):
    fields_by_group = defaultdict(list)
    for field in fields:
        group = resolve_annotation(field['annotations'], 'AzField::Group', '')
        field['group'] = group
        fields_by_group[group].append(field)
    sorted_fields = fields_by_group[""]
    for group, group_fields in fields_by_group.iteritems():
        if group: #ignore the default group, those come first above
            sorted_fields += group_fields
    return sorted_fields

def fixup_class_annotations(object):
    component_menu = resolve_annotation(object['annotations'], 'AzClass::Edit::AppearsInAddComponentMenu')
    if component_menu:
        store_annotation(object['annotations'], 'AzClass::Edit::AppearsInAddComponentMenu', 'AZ_CRC({})'.format(component_menu))

def format_enum_values(enum):
    annotations = enum['annotations']
    values = resolve_annotation(annotations, 'AzEnum::Values')
    if values and values.has_key('AzEnum::Value'):
        values = values['AzEnum::Value']
        for value in values:
            if not value[0].startswith(enum['qualified_name']):
                value[0] = value[0].replace(enum['name'], enum['qualified_name'])
    else:
        elements = enum['elements']
        values = []
        for value in elements:
            values.append([enum['qualified_name'] + '::' + value, quote(value)])

    store_annotation(annotations, 'AzEnum::Values', values, True)


class AzReflectionCPP_Driver(TemplateDriver):

    def apply_transformations(self, json_object):
        format_cpp_annotations(json_object)

        components = []
        nonComponents = []
        needsSerializeContext = False
        needsEditContext = False
        needsBehaviorContext = False

        for object in json_object.get('objects', []):
            # determine serialization/reflection configuration for classes
            configure_reflection(object)
            fixup_class_annotations(object)
            format_description(object['annotations'], 'AzClass', object['name'])

            # collect components/non-components
            if resolve_annotation(object['annotations'], 'AzComponent') is not None:
                components.append(object)
            else:
                nonComponents.append(object)

            object['fields'] = sort_fields_by_group(object.get('fields', []))

            for field in object['fields']:
                format_description(field['annotations'], 'AzField', field['name'])

            if object['serialize']:
                needsSerializeContext = True
            if object['reflectForEdit']:
                needsEditContext = True
            if object['behavior']:
                needsBehaviorContext = True

        for enum in json_object.get('enums', []):
            configure_reflection(enum)
            format_description(enum['annotations'], 'AzEnum', enum['name'])
            format_enum_values(enum)

        json_object['components'] = components
        json_object['classes'] = nonComponents
        json_object['needsSerializeContext'] = needsSerializeContext
        json_object['needsEditContext'] = needsEditContext
        json_object['needsBehaviorContext'] = needsBehaviorContext


    def render_templates(self, input_file, **template_kwargs):
        input_file_name, input_file_ext = os.path.splitext(input_file)
        self.render_template_to_file(
            "AzReflectionCpp.tpl", template_kwargs, '{}.generated.cpp'.format(input_file_name))

# Factory function - called from launcher
def create_drivers(env):
    return [AzReflectionCPP_Driver(env)]
