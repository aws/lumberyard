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
import uuid
import string

from resource_manager.errors import HandledError
from swagger_json_navigator import SwaggerNavigator
QUOTE = "\""
ESCAPE_QUOTE = "\\\""

SWAGGER_TO_CPP_TYPE = {
    "boolean": "bool",
    "integer" : "int",
    "string" : "AZStd::string",
    "number": "double"
}

SWAGGER_TO_CPP_INITIALIZERS = {
    "boolean": "{false}",
    "integer": "{0}",
    "number": "{0.0}"
}

SWAGGER_TO_CS_TYPE = {
    "boolean": "bool",
    "integer" : "int",
    "string" : "string",
    "number": "double"
}

SWAGGER_TO_CS_INITIALIZERS = {
    "boolean": " = false",
    "integer": " = 0",
    "number": " = 0.0"
}

VALID_HTTP_METHODS = [
    "GET",
    "PUT",
    "DELETE",
    "POST"
]

def __get_UUID(source_file, UUID_name):
    if not os.path.isfile(source_file):
        #get new UUID
        return "{" + str(uuid.uuid4()) + "}"
    # read UUID from file to preserve ids through regeneration of the file
    full_def = "const char* LmbrAWS_CodeGen_{}_UUID".format(UUID_name)
    with open(source_file) as file:
        for line in file:
            if full_def in line:
                UUID = line.split('=')[1].strip()
                return UUID[1:-2] # strip of the quotes and semicolon
    return "{" + str(uuid.uuid4()) + "}"


def has_stdafx_files(gem_code_folder):
    expected_path = os.path.join(gem_code_folder, "Source", "StdAfx.h")
    return os.path.exists(expected_path)


def get_UUIDs(source_file, jinja_json):
    # make list of the UUIDs we need
    UUID_names = ["Component",
        "NotificationBus1",
        "RequestBus1",
        "ResponseHandler"]
    for struct in [struct for struct in jinja_json["otherClasses"] if not struct['isArray']]:
        UUID_names.append(struct["name"])

    # generate a UUID for each one
    all_UUIDs = {}
    for UUID_name in UUID_names:
        all_UUIDs[UUID_name] = __get_UUID(source_file, UUID_name)

    return all_UUIDs

def generate_component_json(resource_group, swagger):
    return ComponentJsonBuilder(resource_group, swagger).generate_component_json()

def generate_cs_json(resource_group, swagger):
    return CSJsonBuilder(resource_group, swagger).generate_component_json()

class ComponentJsonBuilder:
    def __init__(self, resource_group, swagger):
        self._swagger = swagger
        self._resource_group_name = resource_group
        self._component_json = {}
        self._swagger_type_map = SWAGGER_TO_CPP_TYPE
        self._swagger_initializer_map = SWAGGER_TO_CPP_INITIALIZERS

    def get_symbol_type(self, swagger_type, default_type):
        return self._swagger_type_map.get(swagger_type, default_type)

    def get_symbol_initializer(self, swagger_type, default_type):
        return self._swagger_initializer_map.get(swagger_type, default_type)

    def get_namespace(self):
        return self._resource_group_name
        
    def generate_component_json(self):
        self.add_def_to_struct_type_conversions()
        self._component_json["namespace"] = self.get_namespace()
        self._component_json["componentClass"] = "{}ClientComponent".format(self._resource_group_name)
        self._component_json["resourceGroup"] = self._resource_group_name
        self._component_json["redefinitions"] = []
        self._component_json["otherClasses"] = []
        self._component_json["functions"] = []

        base_navigator = SwaggerNavigator(self._swagger)
        self.__read_functions(base_navigator.get("paths"))

        return self._component_json


    def __generate_function_name(self, method, path):

        # Use name specified in x-amazon-cloud-canvas-client-generation object,
        # the operationId property or construct one from the path and method.

        function_name = None

        client_generation_extension_object = method.get_object('x-amazon-cloud-canvas-client-generation', default = None)
        if not client_generation_extension_object.is_none:
            function_name = client_generation_extension_object.get_string_value('function', default = None)
            if function_name and not self.__is_valid_cpp_symbol(function_name):
                raise HandledError("x-amazon-cloud-canvas-client-generation.function ({}) for method {} must be a valid C++ symbol".format(function_name, method))

        if not function_name:
            function_name = method.get_string_value('operationId', default = None)
            if function_name and not self.__is_valid_cpp_symbol(function_name):
                raise HandledError("operationId ({}) for method {} must be a valid C++ symbol".format(function_name, method))

        if not function_name:

            path_name = method.selector + path.selector
            path_name = path_name.split("/")

            function_name = ""
            for word in path_name:
                if word[0] in string.letters:
                    word = word[0:1].upper() + word[1:]
                    function_name += word

            if not self.__is_valid_cpp_symbol(function_name):
                raise HandledError("The generated function ({}) for method {} is nto a valid C++ symbol. Use an x-amazon-cloud-canvas-client-generation.function property to override the default.".format(function_name, method))

        return function_name


    def __check_supported_response_type(self, path, response_type):
        if not response_type in [item["name"] for item in self._component_json["otherClasses"]]:
            raise HandledError("{} does not have a object reponse type. The lmbr_aws swagger client generator only supports object response types".format(path))
        if [item for item in self._component_json["otherClasses"] if item["name"] == response_type and item["isArray"]]:
           print "WARNING - {} has an array reponse type. The lmbr_aws swagger client generator only supports object response types".format(path)

    def __read_functions(self, paths):
        for path in paths.values():
            for method in path.values():
                if not method.selector.upper() in VALID_HTTP_METHODS:
                    continue
                if method.get("x-amazon-cloud-canvas-client-generation", {}).get("no-client", False).value == True:
                    continue
                func_def = {}

                function_name = self.__generate_function_name(method, path)
                func_def["functionName"] = function_name

                param_list = [] # for method definition
                param_name_list = [] # for WriteJson body (should not include query or path params)
                path_params = [] # for request.SetPathParameter
                query_params = [] # for request.AddQueryParameter
                signature_params = [] # for function signature

                params = method.get("parameters", {})

                for param in params.values():
                    if not param.get("in", ""):
                        raise HandledError("{} has no 'in' property".format(param))
                    param_type = self.__get_param_type(function_name, param)
                    param_list.append("{} {}".format(self.get_symbol_type(param_type, param_type), param.get("name").value))
                    signature_params.append("const {}& {}".format(self.get_symbol_type(param_type, param_type), param.get("name").value))

                    if param.get("in").value == "body":
                        param_name_list.append(param.get("name").value)
                    elif param.get("in").value == "query":
                        query_params.append(param.get("name").value)
                    elif param.get("in").value == "path":
                        path_params.append(param.get("name").value)
                    else:
                        raise HandledError("{} has invalid 'in' property: {}".format(param, param.get("in").value))

                func_def["path"] = path.selector
                func_def["http_method"] = method.selector.upper()
                func_def["queryParamNames"] = query_params
                func_def["pathParamNames"] = path_params
                func_def["paramNames"] = param_name_list
                func_def["params"] = param_list
                func_def["typedParams"] = ", ".join(signature_params)
                response_type = self.__get_response_type(function_name, method.get("responses"))
                if response_type:
                    self.__check_supported_response_type(path, response_type)
                    func_def["responseType"] = self.get_symbol_type(response_type, response_type)

                self._component_json["functions"].append(func_def)



    def __get_response_type(self, function_name, responses):
        success_response = responses.get("200", {})

        if not success_response.value:
            return None

        response_schema = success_response.get("schema", {})

        if not response_schema.value:
            return None

        raw_type = self.__get_raw_response_type(function_name, response_schema)
        if not "type" in response_schema.value:
            if response_schema.get("$ref", []).value:
                type_name = raw_type.split("/")[-1]
                if not self.__type_defined(type_name):
                    self.__generate_from_schema(type_name, response_schema.resolve_ref(response_schema.get("$ref", []).value))
                return type_name
        return raw_type


    def __get_raw_response_type(self, function_name, response_schema):
        return_type = response_schema.get("type", "").value

        generated_name = "{}ReturnType".format(function_name)

        if return_type == "array":
            if not self.__type_defined(generated_name):
                self.__generate_from_schema(generated_name, response_schema)
            return generated_name

        if return_type == "object":
            if not self.__type_defined(generated_name):
                self.__generate_from_schema(generated_name, param)
            return generated_name

        if not return_type:
            if "$ref" in response_schema.value:
                return response_schema.get("$ref").value
            if not self.__type_defined(generated_name):
                self.__generate_from_schema(generated_name, response_schema)
            return generated_name

        return return_type


    def __get_param_type(self, function_name, param):
        raw_type = self.__get_raw_param_type(function_name, param)
        if not "type" in param.value and "schema" in param.value:
            schema = param.get("schema")
            if schema.get("$ref", []).value:
                type_name = raw_type.split("/")[-1]
                if not self.__type_defined(type_name):
                    self.__generate_from_schema(type_name, schema.resolve_ref(schema.get("$ref", []).value))
                return type_name
        return raw_type


    def __get_raw_param_type(self, function_name, param):
        param_type = param.get("type", "").value
        generated_name = "{}Param{}".format(function_name, param.get("name").value)
        if param_type == "array":
            if not self.__type_defined(generated_name):
                self.__generate_from_schema(generated_name, param)
            return generated_name

        if param_type == "object":
            if not self.__type_defined(generated_name):
                self.__generate_from_schema(generated_name, param)
            return generated_name

        if not param_type: # there must be a schema definition here
            schema = param.get("schema", {})
            if not schema.value:
                raise HandledError("No param type given for {}".format(param))
            if "$ref" in schema.value:
                return schema.get("$ref").value
            if not self.__type_defined(generated_name):
                self.__generate_from_schema(generated_name, schema)
            return generated_name

        return param_type


    def __get_all_definitions(self):
        return self._component_json["otherClasses"] + self._component_json["redefinitions"]


    def __type_defined(self, name):
        for item in self.__get_all_definitions():
            if item.get("name", "") == name:
                return True
        return False


    def __find_definition(self, name):
        for item in self.__get_all_definitions():
            if item.get("name", "") == name:
                return item
        raise HandledError("No definition found for {}".format(name))


    def __get_type(self, schema):
        if "type" in schema:
            return schema["type"]
        return "object" # seems that the proper default is object


    def __generate_object_from_schema(self, name, schema):
        obj_def = {}
        obj_def["name"] = name
        obj_def["props"] = []
        obj_def["isArray"] = False
        for item in schema.get("allOf", []).values():
            if "$ref" in item.value:
                ref_path = item.value["$ref"]
                ref_name = ref_path.split("/")[-1]
                if not self.__type_defined(ref_name):
                    ref = item.resolve_ref(ref_path)
                    self.__generate_from_schema(ref_name, ref)
                ref_def = self.__find_definition(ref_name)
                obj_def["props"] += ref_def["props"]

            for prop_name in item.get("properties", {}).value:
                prop = self.__read_property(item.get("properties", {}), prop_name, name)
                obj_def["props"].append(prop)

        properties = schema.get("properties", {})
        for prop_name in properties.value:
            prop = self.__read_property(properties, prop_name, name)
            obj_def["props"].append(prop)

        self.__add_struct_def(obj_def)

    def __read_property(self, properties, prop_name, obj_name):
        prop = {}
        prop["name"] = prop_name
        if "type" in properties.get(prop_name).value:
            def_type = properties.get(prop_name).get("type").value
            if def_type in ["array", "object"]:
                auto_generated_name = "{}Property{}".format(obj_name[0].upper() + obj_name[1:], prop_name[0].upper() + prop_name[1:])
                self.__generate_from_schema(auto_generated_name, properties.get(prop_name))
                prop["type"] = auto_generated_name
                prop["init"] = ""
            else:
                prop["type"] = self.get_symbol_type(def_type, def_type)
                prop["init"] = self.get_symbol_initializer(def_type, "")

        elif "$ref" in properties.get(prop_name).value: # must be a ref
            item = properties.get(prop_name)
            ref_path = item.value['$ref']
            ref_name = ref_path.split("/")[-1]
            if not self.__type_defined(ref_name):
                ref = item.resolve_ref(ref_path)
                self.__generate_from_schema(ref_name, ref)
            prop["type"] = ref_name
        else:
            raise HandledError("no type found for property {} in definition at {}".format(prop_name, properties))
        return prop

    def __add_struct_def(self, obj_def):
        self._component_json["otherClasses"].append(obj_def)


    def __generate_array_from_schema(self, name, schema):
        array_def = {}
        array_def["name"] = name
        array_def["isArray"] = True
        # items is a required field
        items = schema.get("items")
        if "$ref" in items.value:
            ref_path = items.value["$ref"]
            ref_name = ref_path.split("/")[-1]
            if not self.__type_defined(ref_name):
                ref = items.resolve_ref(ref_path)
                self.__generate_from_schema(ref_name, ref)
            elements = items.get("$ref").value
        else:
            elements = items.get("type").value

        array_def["elements"] = self.get_symbol_type(elements, elements)
        self._component_json["otherClasses"].append(array_def)


    def __generate_redef(self, name, def_type):
        redef = {}
        redef["primitiveType"] = self.get_symbol_type(def_type, def_type)
        redef["name"] = name
        self._component_json["redefinitions"].append(redef)


    def __generate_from_schema(self, name, schema, json_path = []):
        # here we don't know what it looks like. Could be primitive, object, or array
        if not json_path:
            json_path = ["definitions", name]
        def_type = self.__get_type(schema.value)
        if def_type == "object":
            self.__generate_object_from_schema(name, schema)
        elif def_type == "array":
            self.__generate_array_from_schema(name, schema)
        else: # it is a redefinition
            self.__generate_redef(name, def_type)


    def __is_valid_cpp_symbol(self, symbol):
        if not symbol[0] in string.letters + "_":
            return False
        for char in symbol:
            if not char in string.letters + string.digits + "_":
                return False
        return True

    def add_def_to_struct_type_conversions(self):
        for def_name, info in self._swagger.get("definitions", {}).iteritems():
            this_type = info.get("type", "")
            if this_type in ["string", "boolean", "integer", "number"]:
                self._swagger_type_map[def_name] = self._swagger_type_map[this_type]
                self._swagger_type_map["#/definitions/{}".format(def_name)] = self._swagger_type_map[this_type]
            else:
                self._swagger_type_map["#/definitions/{}".format(def_name)] = def_name

class CSJsonBuilder(ComponentJsonBuilder):

    def __init__(self, resource_group, swagger):
        self._swagger = swagger
        self._resource_group_name = resource_group
        self._component_json = {}
        self._swagger_type_map = SWAGGER_TO_CS_TYPE
        self._swagger_initializer_map = SWAGGER_TO_CS_INITIALIZERS

    def get_namespace(self):
        return self._resource_group_name.replace('CloudGem','CloudCanvas.')
