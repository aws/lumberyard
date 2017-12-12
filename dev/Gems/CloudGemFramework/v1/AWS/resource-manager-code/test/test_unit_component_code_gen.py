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

import unittest
import mock
import copy

import component_gen_utils

TEST_SWAGGER = \
{
    "swagger": "2.0",
    "info": {
        "version": "1.0.0",
        "title": "TestSwagger",
        "description": "API for testing the component generator"
   },
    "schemes": [
        "https"
    ],
    "consumes": [
        "application/json"
    ],
    "produces": [
        "application/json"
    ],
    "paths": {
    },
    "definitions": {
    }
}
class UnitTest_CloudGemFramework_ResourceManagerCode_ComponentCodeGen(unittest.TestCase):


    def __find_object_index(self, jinja_json, object_name):
        for index in range(0, len(jinja_json["otherClasses"])):
            if jinja_json["otherClasses"][index]["name"] == object_name:
                return index
        raise AssertionError("{} does not exist in the given jinja_json".format(object_name))


    def __setup_swagger(self):
        swagger = copy.deepcopy(TEST_SWAGGER)


        swagger["paths"]["/test/path"] = {
            "get": {
                "operationId": "test_path",
                "description": "just a test",
                "responses": {
                    "200": {
                        "description": "a response",
                        "schema": {
                            "$ref": "#/definitions/objectA"
                        }
                    }
                }
            }
        }

        return swagger


    def test_order_of_definititions(self):
        swagger = self.__setup_swagger()
        # given two definitions where the first depends on the second
        swagger["definitions"]["objectA"] = {
            "type": "object",
            "properties": {
                "objectDep": {
                    "$ref": "#/definitions/objectB"
                }
            }
        }

        swagger["definitions"]["objectB"] = {
            "type": "object",
            "properties": {
                "data": {
                    "type": "string"
                }
            }
        }

        # the objects should be listed after their dependencies
        test_jinja_json = component_gen_utils.generate_component_json("TestGroup", swagger)
        self.assertEquals(1, self.__find_object_index(test_jinja_json, "objectA"))
        self.assertEquals(0, self.__find_object_index(test_jinja_json, "objectB"))


    def test_undefinied_ref_fails(self):
        swagger = self.__setup_swagger()
        hit_error = False
        # Given a swagger with a reference to #/definitions/objectA
        try:
            test_jinja_json = component_gen_utils.generate_component_json("TestGroup", swagger)
        except ValueError as e:
            # fail on ValueError if objectA is not defined
            hit_error = True

        # fail on ValueError if objectA is not defined
        self.assertTrue(hit_error, "Error was not hit for unresolvable definition")


    def test_auto_generate_property_type(self):
        AUTO_GENERATED_PROPERTY_NAME = 'ObjectAPropertyInlineArrayProp'
        swagger = self.__setup_swagger()
        # given an object with a non-primitive property defined inline
        swagger["definitions"]["objectA"] = {
            "type": "object",
            "properties": {
                "inlineArrayProp": {
                    "type": "array",
                    "items": {
                        "type": "string"
                    }
                }
            }
        }

        test_jinja_json = component_gen_utils.generate_component_json("TestGroup", swagger)
        # make sure that the inline is defined first (because of objectA's dependency on it)
        self.assertEquals(0, self.__find_object_index(test_jinja_json, AUTO_GENERATED_PROPERTY_NAME))
        # make sure it is an array
        self.assertTrue(test_jinja_json["otherClasses"][0]["isArray"], "inline definied array was not marked as array")
        # make sure object A is defined
        self.assertEquals(1, self.__find_object_index(test_jinja_json, "objectA"))
