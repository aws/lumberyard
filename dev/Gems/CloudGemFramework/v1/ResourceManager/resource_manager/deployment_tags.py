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

from errors import HandledError

import json
import os
import copy
from resource_manager_common import constant


class DeploymentTag():
    def __init__(self, deployment_name, context):
        self._context = context
        self._tags = context.config.local_project_settings.get(constant.DEPLOYMENT_TAGS, {}).get(deployment_name, [])
        self.path_found = []


    def get_tag_overrides(self, resource_group_name):
        all_overrides = {}
        for tag in self._tags:
            tag_overrides = self.load_tag(tag)
            if resource_group_name in tag_overrides:
                for override_key in tag_overrides[resource_group_name]:
                    if override_key in all_overrides:
                        raise HandledError("An override for {} in {} was already provided".format(override_key, resource_group_name))
                    else:
                        all_overrides[override_key] = tag_overrides[resource_group_name][override_key]
                self._context.view._output_message("Applying override from tag \"{}\" to resource group {}".format(tag, resource_group_name))
        return all_overrides


    def apply_overrides(self, resource_group):
        overrides = self.get_tag_overrides(resource_group.name)
        template = copy.deepcopy(resource_group.template)
        for override_name, override_data in overrides.iteritems():
            operation = override_data.get("EditType", "")
            if operation == "Replace":
                template = self.replace_template_content(
                    override_name, override_data.get("TemplateData", {}), template)
            elif operation == "Add":
                template = self.add_template_content(
                    override_name, override_data.get("TemplateData", {}), template)
            elif operation == "Delete":
                template = self.remove_template_content(override_name, template)
            else:
                raise HandledError("Unkonwn operation, DeploymentTag operation {} is not valid".format(operation))
        return template


    def validate_json_path(self, check_template, path_list):
        i = 0
        while i < len(path_list):
            if not path_list[i] in check_template:
                raise HandledError(
                    "Path {} does not exist in template file".format(path_list[:i+1]))
            check_template = check_template[path_list[i]]
            i = i + 1

    def replace_template_content(self, override_name, override_template_data, template):
        path_list = override_name.split(":")
        self.validate_json_path(template, path_list)
        return self.recursive_edit(path_list, override_template_data, template, "Replace")


    def add_template_content(self, override_name, override_template_data, template):
        path_list = override_name.split(":")
        self.path_found = []
        new_template =  self.recursive_edit(copy.deepcopy(path_list), override_template_data, template, "Add")
        if self.path_found != path_list:
            message = "Template did not contain the object we were looking for at {}. Object was created to add content at {}".format(self.path_found, path_list)
            self._context.view._output_message(message)
        return new_template


    def remove_template_content(self, override_name, template):
        path_list = override_name.split(":")
        self.validate_json_path(template, path_list)
        return self.recursive_edit(path_list, {}, template, "Delete")


    def recursive_edit(self, path_list, override_template_data, json_data, operation):
        if len(path_list) == 1:
            if operation == "Delete":
                del json_data[path_list[0]]
            else:
                json_data[path_list[0]] = override_template_data
            return json_data

        new_key = path_list.pop(0)
        if not isinstance(json_data, dict):
            raise HandledError("Could not find key {}, was expecting json object to be dict, found {}".format(new_key, type(json_data)))

        if new_key in json_data:
            self.path_found.append(new_key)
        json_data[new_key] = self.recursive_edit(
            path_list, override_template_data, json_data.get(new_key, {}), operation)
        return json_data


    def load_tag(self, tag):
        overrides = {}
        tags_file_path = os.path.join(self._context.config.aws_directory_path, "deployment-tags", "{}.json".format(tag))
        if not os.path.exists(tags_file_path):
            self._context.view._output_message("Could not find file for tag {}".format(tag))
            return overrides

        with open(tags_file_path, 'r') as f:
            overrides = json.load(f)

        return overrides
