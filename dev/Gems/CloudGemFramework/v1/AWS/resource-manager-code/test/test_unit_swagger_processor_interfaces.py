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
import mock
import unittest

from mock_swagger_json_navigator import SwaggerNavigatorMatcher
from swagger_json_navigator import SwaggerNavigator
from cgf_utils.version_utils import Version

import swagger_processor.interface
import swagger_processor.lambda_dispatch
import resource_manager_common.service_interface

class AnyOrderListMatcher(object):

    def __init__(self, expected_value):
        self.__expected_value = expected_value

    def __eq__(self, other):
        return sorted(other) == sorted(self.__expected_value)

    def __repr__(self):
        return str(self.__expected_value) + ' (in any order)'


class UnitTest_CloudGemFramework_ResourceManagerCode_swagger_processor_interfaces(unittest.TestCase):


    def __init__(self, *args, **kwargs):
        super(UnitTest_CloudGemFramework_ResourceManagerCode_swagger_processor_interfaces, self).__init__(*args, **kwargs)


    def setUp(self):

        self.mock_context = mock.MagicMock('context')

        self.target_swagger = {
            "swagger": "2.0",
            "info": {
                "version": "1.0.0",
                "title": ""
           },
           "paths": {
            }
        }

        self.target_swagger_navigator = SwaggerNavigator(self.target_swagger)

        self.interface_swagger = {
            "swagger": "2.0",
            "info": {
                "version": "1.0.0",
                "title": ""
           },
           "paths": {
            }
        }

        self.interface_swagger_navigator = SwaggerNavigator(self.interface_swagger)

        self.test_interface_id = 'test_interface_1_2_3'
        self.test_path = '/test_path'
        self.test_definition_name = 'test-definition-name'
        self.test_definition_value = { 'test-definition-property': 'test-definition-value' }
        self.test_mappings = {}
        self.test_lambda_dispatch_for_paths = {}
        self.test_external_parameters = None

    def add_target_path(self, path, content = None):
        content = content or {}
        path_object = self.target_swagger_navigator.get_object('paths').add_object(path, content)
        return path_object


    def add_interface_path(self, path, content = None):
        content = content or {}
        path_object = self.interface_swagger_navigator.get_object('paths').add_object(path, content)
        return path_object


    def add_target_interface_implementation_object(self, path, interface, path_content = None, lambda_dispatch = None, external_parameters = None):
        
        if lambda_dispatch:
            interface_implementation_object_value = {
                'interface': {
                    'id': interface,
                    'lambda_dispatch': lambda_dispatch
                }
            }
        else:
            interface_implementation_object_value = {
                'interface': interface
            }

        path_object = self.add_target_path(path, path_content)

        interface_implementation_object = path_object.add_object(
            resource_manager_common.service_interface.INTERFACE_IMPLEMENTATION_OBJECT_NAME, 
            interface_implementation_object_value)

        if external_parameters:
            path_object.get_or_add_array('parameters', default = external_parameters)

        return interface_implementation_object


    def add_interface_definition(self, definition_name, definition_value):
        definitions_object = self.interface_swagger_navigator.get_or_add_object('definitions')
        definitions_object.value[definition_name] = definition_value
        return definitions_object.get(definition_name)


    def add_target_definition(self, definition_name, definition_value):
        definitions_object = self.target_swagger_navigator.get_or_add_object('definitions')
        definitions_object.value[definition_name] = definition_value


    @mock.patch('swagger_processor.interface.process_interface_implementation_object')
    def test_process_interface_implementation_objects_processes_all_objects(self, mock_process_interface_implementation_object):

        self.add_target_path('/path-without-interface-implemenation')

        interface_implementation_object_1 = self.add_target_interface_implementation_object(
            self.test_path + '1', 
            self.test_interface_id)

        interface_implementation_object_2 = self.add_target_interface_implementation_object(
            self.test_path + '2', 
            self.test_interface_id)

        swagger_processor.interface.process_interface_implementation_objects(
            self.mock_context, 
            self.target_swagger_navigator)

        mock_process_interface_implementation_object.assert_has_calls(
            [ 
                mock.call(self.mock_context, SwaggerNavigatorMatcher(interface_implementation_object_1)),
                mock.call(self.mock_context, SwaggerNavigatorMatcher(interface_implementation_object_2))
            ],
            any_order = True)
        

    @mock.patch('swagger_processor.interface.load_interface_swagger')
    @mock.patch('swagger_processor.interface.insert_interface_definitions')
    @mock.patch('swagger_processor.interface.insert_interface_paths')
    def test_process_interface_implementation_object(self, mock_insert_interface_paths, mock_insert_interface_definitions, mock_load_interface_swagger):

        mock_lambda_dispatch = {
            'paths': {}
        }

        mock_external_parameters = []

        interface_implementation_object = self.add_target_interface_implementation_object(
            self.test_path, 
            self.test_interface_id,
            lambda_dispatch = mock_lambda_dispatch,
            external_parameters = mock_external_parameters)

        mock_mappings = mock_insert_interface_definitions.return_value
        mock_interface_swagger = mock_load_interface_swagger.return_value

        swagger_processor.interface.process_interface_implementation_object(
            self.mock_context, 
            interface_implementation_object)

        mock_load_interface_swagger.assert_called_once_with(
            self.mock_context, 
            self.test_interface_id)

        mock_insert_interface_definitions.assert_called_once_with(
            SwaggerNavigatorMatcher(interface_implementation_object.root), 
            mock_interface_swagger,
            self.test_interface_id,
            mock_external_parameters)

        mock_insert_interface_paths.assert_called_once_with(
            SwaggerNavigatorMatcher(interface_implementation_object.parent), 
            mock_interface_swagger,
            self.test_interface_id,
            mock_mappings,
            mock_lambda_dispatch['paths'],
            mock_external_parameters)

        self.assertEquals(
            self.target_swagger[resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME], 
            { 
                self.test_interface_id: {
                    'basePath': self.test_path
                }
            }
        )



    def test_insert_interface_definitions_with_no_source_or_target_definitions_does_not_add_definitions(self):
        
        external_parameters = {}

        swagger_processor.interface.insert_interface_definitions(
            self.target_swagger_navigator, 
            self.interface_swagger_navigator,
            self.test_interface_id,
            external_parameters)
        
        self.assertNotIn('definitions', self.target_swagger.keys())
        self.assertNotIn(resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME, self.target_swagger.keys())
                         


    def test_insert_interface_definitions_with_only_target_definitions_does_not_modify_definitions(self):
        
        self.add_target_definition(self.test_definition_name, self.test_definition_value)
        target_definitions_copy = copy.deepcopy(self.target_swagger_navigator.get_object('definitions').value)

        external_parameters = {}

        swagger_processor.interface.insert_interface_definitions(
            self.target_swagger_navigator, 
            self.interface_swagger_navigator,
            self.test_interface_id,
            external_parameters)
        
        self.assertEquals(self.target_swagger_navigator.get_object('definitions').value, target_definitions_copy)
        self.assertNotIn(resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME, self.target_swagger.keys())


    def test_insert_interface_definitions_with_multiple_source_definitions_inserts_all_definitions(self):

        self.add_interface_definition(self.test_definition_name + '1', self.test_definition_value)
        self.add_interface_definition(self.test_definition_name + '2', self.test_definition_value)

        external_parameters = {}

        swagger_processor.interface.insert_interface_definitions(
            self.target_swagger_navigator, 
            self.interface_swagger_navigator,
            self.test_interface_id,
            external_parameters)

        target_definitions_object = self.target_swagger_navigator.get_object('definitions')

        result_definition = target_definitions_object.get_object(self.test_definition_name + '1')
        self.assertEquals(result_definition.value, self.test_definition_value)

        result_definition = target_definitions_object.get_object(self.test_definition_name + '2')
        self.assertEquals(result_definition.value, self.test_definition_value)

        self.assertEquals(
            self.target_swagger[resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME], 
            { 
                self.test_interface_id: {
                    'definitions': AnyOrderListMatcher(
                        [
                            self.test_definition_name + '1',
                            self.test_definition_name + '2'
                        ]
                    )
                }
            }
        )


    def test_insert_interface_definitions_preserves_existing_definition(self):

        interface_definition = { 'interface': 'definition' }
        self.add_interface_definition(self.test_definition_name, interface_definition)

        target_definition = { 'target': 'definition' }
        self.add_target_definition(self.test_definition_name, target_definition)

        external_parameters = []

        swagger_processor.interface.insert_interface_definitions(
            self.target_swagger_navigator, 
            self.interface_swagger_navigator,
            self.test_interface_id,
            external_parameters)

        target_definitions_object = self.target_swagger_navigator.get_object('definitions')

        result_definition = target_definitions_object.get_object(self.test_definition_name + '1')
        self.assertEquals(result_definition, SwaggerNavigatorMatcher(interface_definition))

        result_definition = target_definitions_object.get_object(self.test_definition_name)
        self.assertEquals(result_definition, SwaggerNavigatorMatcher(target_definition))
        
        self.assertEquals(
            self.target_swagger[resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME], 
            { 
                self.test_interface_id: {
                    'definitions': [
                        self.test_definition_name + '1'
                    ]
                }
            })



    def test_insert_interface_definitions_generates_unique_defintion_names(self):

        interface_definition = { 'interface': 'definition' }
        self.add_interface_definition(self.test_definition_name, interface_definition)

        target_definition = { 'target': 'definition' }
        self.add_target_definition(self.test_definition_name, target_definition)

        target_definition_1 = { 'target': 'definition1' }
        self.add_target_definition(self.test_definition_name + '1', target_definition_1)

        target_definition_2 = { 'target': 'definition2' }
        self.add_target_definition(self.test_definition_name + '2', target_definition_2)

        external_parameters = []

        swagger_processor.interface.insert_interface_definitions(
            self.target_swagger_navigator, 
            self.interface_swagger_navigator,
            self.test_interface_id,
            external_parameters)

        target_definitions_object = self.target_swagger_navigator.get_object('definitions')

        result_definition = target_definitions_object.get_object(self.test_definition_name + '3')
        self.assertEquals(result_definition, SwaggerNavigatorMatcher(interface_definition))

        result_definition = target_definitions_object.get_object(self.test_definition_name + '1')
        self.assertEquals(result_definition, SwaggerNavigatorMatcher(target_definition_1))
        
        result_definition = target_definitions_object.get_object(self.test_definition_name + '2')
        self.assertEquals(result_definition, SwaggerNavigatorMatcher(target_definition_2))
        
        result_definition = target_definitions_object.get_object(self.test_definition_name)
        self.assertEquals(result_definition, SwaggerNavigatorMatcher(target_definition))
        
        self.assertEquals(
            self.target_swagger[resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME], 
            { 
                self.test_interface_id: {
                    'definitions': [
                        self.test_definition_name + '3'
                    ]
                }
            })


    def test_insert_interface_paths_inserts_paths(self):

        interface_paths = ['/foo', '/bar']
        interface_paths_content = {}
        lambda_dispatch_for_paths = {}
        expected_interface_paths_content = {}
        external_parameters = []

        for interface_path in interface_paths:
            interface_path_content = { 'get': { 'interface_path': interface_path }, 'parameters': [ { 'internal': 'parameter' } ] }
            lambda_dispatch_for_path = { 'get': { 'lambda_dispatch': interface_path } }
            interface_paths_content[interface_path] = interface_path_content
            self.add_interface_path(interface_path, interface_path_content)
            lambda_dispatch_for_paths[interface_path] = lambda_dispatch_for_path
            expected_interface_paths_content[interface_path] = copy.deepcopy(interface_path_content)
            expected_interface_paths_content[interface_path]['get'][swagger_processor.lambda_dispatch.LAMBDA_DISPATCH_OBJECT_NAME] = lambda_dispatch_for_path['get']
            expected_parameters = []
            expected_parameters.extend(external_parameters)
            expected_parameters.extend(interface_path_content['parameters'])
            expected_interface_paths_content[interface_path]['parameters'] = AnyOrderListMatcher(expected_parameters)

        target_path_content = { 'target_path': self.test_path }

        interface_implementation_object = self.add_target_interface_implementation_object(
            self.test_path, 
            self.test_interface_id,
            path_content = target_path_content)

        swagger_processor.interface.insert_interface_paths(
            interface_implementation_object.parent,
            self.interface_swagger_navigator,
            self.test_interface_id,
            self.test_mappings,
            lambda_dispatch_for_paths,
            external_parameters)

        target_paths = self.target_swagger_navigator.get_object('paths')

        self.assertTrue(target_paths.contains(self.test_path))
        self.assertEqual(
            target_paths.get_object(self.test_path).value, 
            target_path_content)

        for interface_path in interface_paths:
            self.assertTrue(target_paths.contains(self.test_path + interface_path))
            self.assertEquals(
                target_paths.get_object(self.test_path + interface_path).value, 
                expected_interface_paths_content[interface_path])

        self.assertEquals(
            self.target_swagger[resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME], 
            { 
                self.test_interface_id: {
                    'paths': AnyOrderListMatcher(
                        [ self.test_path + interface_path for interface_path in interface_paths]
                    )
                }
            }
        )


    def test_insert_interface_paths_works_on_root_path(self):

        interface_paths = ['/foo', '/bar']
        interface_paths_content = {}

        for interface_path in interface_paths:
            interface_path_content = { 'interface_path': interface_path }
            interface_paths_content[interface_path] = interface_path_content
            self.add_interface_path(interface_path, interface_path_content)

        target_path_content = { 'target_path': self.test_path }

        interface_implementation_object = self.add_target_interface_implementation_object(
            '/', 
            self.test_interface_id,
            path_content = target_path_content)

        swagger_processor.interface.insert_interface_paths(
            interface_implementation_object.parent,
            self.interface_swagger_navigator,
            self.test_interface_id,
            self.test_mappings,
            self.test_lambda_dispatch_for_paths,
            self.test_external_parameters)

        target_paths = self.target_swagger_navigator.get_object('paths')

        self.assertTrue(target_paths.contains('/'))
        self.assertEqual(
            target_paths.get_object('/').value, 
            target_path_content)

        for interface_path in interface_paths:
            self.assertTrue(target_paths.contains(interface_path))
            self.assertEquals(
                target_paths.get_object(interface_path).value, 
                interface_paths_content[interface_path])

        self.assertEquals(
            self.target_swagger[resource_manager_common.service_interface.INTERFACE_METADATA_OBJECT_NAME], 
            { 
                self.test_interface_id: {
                    'paths': AnyOrderListMatcher(interface_paths)
                }
            })


    def test_insert_interface_paths_fails_with_duplicate_paths(self):

        interface_path = '/foo'
        self.add_interface_path(interface_path)

        interface_implementation_object = self.add_target_interface_implementation_object(
            self.test_path, 
            self.test_interface_id)

        self.add_target_path(self.test_path + interface_path)

        with self.assertRaisesRegexp(RuntimeError, self.test_path + interface_path):
            swagger_processor.interface.insert_interface_paths(
                interface_implementation_object.parent,
                self.interface_swagger_navigator,
                self.test_interface_id,
                self.test_mappings,
                self.test_lambda_dispatch_for_paths,
                self.test_external_parameters)


    def test_copy_with_updated_refs_with_ref(self):

        test_target = '#/definition/test-target'
        definition_object = self.add_interface_definition(self.test_definition_name, { '$ref': test_target })
        mappings = {'test-target': 'mapped-target'}

        result = swagger_processor.interface.copy_with_updated_refs(definition_object, mappings)
        self.assertEquals(result, { '$ref': '#/definition/mapped-target' })


    def test_copy_with_updated_refs_with_nested_ref_in_object(self):

        test_target = '#/definition/test-target'
        definition_object = self.add_interface_definition(self.test_definition_name, { 'container': { 'the-ref': {'$ref': test_target } } })
        mappings = {'test-target': 'mapped-target'}

        result = swagger_processor.interface.copy_with_updated_refs(definition_object, mappings)

        self.assertEquals(result, { 'container': { 'the-ref': {'$ref': '#/definition/mapped-target' } } })


    def test_copy_with_updated_refs_with_nested_ref_in_array(self):

        test_target = '#/definition/test-target'
        definition_object = self.add_interface_definition(self.test_definition_name, { 'container': [ {'$ref': test_target } ] })
        mappings = {'test-target': 'mapped-target'}

        result = swagger_processor.interface.copy_with_updated_refs(definition_object, mappings)

        self.assertEquals(result, { 'container': [ {'$ref': '#/definition/mapped-target' } ] })


    def test_copy_with_updated_refs_with_nested_ref_in_object_in_array(self):

        test_target = '#/definition/test-target'
        definition_object = self.add_interface_definition(self.test_definition_name, { 'container': [ { 'the-ref': {'$ref': test_target } } ] })
        mappings = {'test-target': 'mapped-target'}

        result = swagger_processor.interface.copy_with_updated_refs(definition_object, mappings)

        self.assertEquals(result, { 'container': [ { 'the-ref': {'$ref': '#/definition/mapped-target' } } ] })


    def test_copy_with_updated_refs_with_string(self):
        mappings = { 'string': 'should-not-be-used' }
        string_value = 'string'
        string_navigator = self.add_interface_definition(self.test_definition_name, 'string')
        result = swagger_processor.interface.copy_with_updated_refs(string_navigator, mappings)
        self.assertEquals(result, string_value)


    def test_parse_interface_id_with_valid_id(self):

        expected_gem_name = 'TestGemName'
        expected_interface_name = 'TestInterfaceName'
        expected_interface_version = Version('1.2.3')

        interface_id = expected_gem_name + '_' + expected_interface_name + '_' + str(expected_interface_version).replace('.', '_')

        actual_gem_name, actual_interface_name, actual_interface_version = swagger_processor.interface.parse_interface_id(interface_id)

        self.assertEquals(actual_gem_name, expected_gem_name)
        self.assertEquals(actual_interface_name, expected_interface_name)
        self.assertEquals(actual_interface_version, expected_interface_version)

