#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

# @package az_code_gen.base
# Contains the base class for code generation template drivers.
# Handles the initialization of the template engine and common functionality
# to simplify derived scripts.

import json
import os
import sys
import jinja_extensions
import jinja2
from azcg_extension import *

class AZCGError(Exception):
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return repr(self.value)


# Base template driver behavior class, code generation scripts must inherit from this class
class TemplateDriver:
    def __init__(self, env):
        self.env = env

    # Override this method to invoke your template rendering
    def render_templates(self, input_file, **template_kwargs):
        pass

    # ---------------------------------------------------
    # Should run
    def should_run(self, data_object):
        return True

    # ---------------------------------------------------
    # Derived classes may implement this to perform custom processing and invoke the template engine.
    # @return boolean True if execution was successful, false if execution failed
    # @param data_object The data object in the format sent by the codegen utility to use.
    # @param input_file The input file being parsed for the intermediate data
    def execute(self, data_object, input_file):
        # Load and parse the provided data in data_object (currently json, this may change).
        json_object = json.loads(data_object)

        # allow TemplateDriver subclass to alter the object before rendering
        extra_data = self.apply_transformations(json_object)

        # Check if we should run (typically looks for known tags)
        if self.should_run(json_object):
            # Create a raw json string of the above data we processed
            json_str = json.dumps(json_object, indent=4)
            extra_str = json.dumps(extra_data, indent=4)
            error_msg = self.render_templates(input_file, json_object=json_object, json_str=json_str, extra_data=extra_data, extra_str=extra_str)
            if error_msg:
                OutputError("render_templates returned failed with message \"{}\" for Template Driver {} in module {} executing on input file {}".format(error_msg, self.__class__.__name__, self.__class__.__module__, input_file))
                return False
        return True

    # Call this method to run jinja and write output to disk
    def render_template_to_file(self, template_file, template_kwargs, output_file, should_add_to_build=False):
        jinja_template = self.load_template(template_file)
        if jinja_template is None:
            return
        generated_output = jinja_template.render(**template_kwargs)
        self.env.output_writer(output_file, generated_output, should_add_to_build)

    # Call this method to notify the code generator that if the provided file changes on disk, we must re-run the current code generator task
    def add_dependency(self, dependency_file):
        RegisterDependencyFile(dependency_file)

    # ---------------------------------------------------
    # Load the template, we expect this to be provided by the code generation utility.
    def load_template(self, template_file):
        # Prepare the template for future rendering
        return self.env.jinja_env.get_template(template_file)

    # ---------------------------------------------------
    # Stub for subclasses to override to make any arbitrary changes to the object.
    def apply_transformations(self, obj):
        pass


# Class that handles the multiple TemplateDriver support, it behaves as a TemplateDriver factory
#  loading script files as modules.
class Environment:
    def __init__(self):
        self.modules = {}

    # Given a loaded script module, invoke the script's "create_drivers" function and return them
    def get_drivers_for_script(self, script_file):
        if not os.path.exists(script_file):
            raise ImportError(script_file)

        module_path, script_name = os.path.split(script_file)
        module_name, script_ext = os.path.splitext(script_name)
        sys.path = [module_path] + sys.path
        module = __import__(module_name)
        return module.create_drivers(self)



# ---------------------------------------------------
# Utility class to emulate enum behavior
class Enum(set):
    def __getattr__(self, name):
        if name in self:
            return name
        raise AttributeError


# ---------------------------------------------------
def create_environment():
    env = Environment()
    return env
