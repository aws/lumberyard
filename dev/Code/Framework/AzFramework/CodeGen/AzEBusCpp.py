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

class AzEBusCPP_Driver(TemplateDriver):

    def apply_transformations(self, json_object):
        format_cpp_annotations(json_object)

    def render_templates(self, input_file, **template_kwargs):
        input_file_name, input_file_ext = os.path.splitext(input_file)
        self.render_template_to_file("AzEBusCpp.tpl", template_kwargs, '{}.generated.cpp'.format(input_file_name))


# Factory function - called from launcher
def create_drivers(env):
    return [AzEBusCPP_Driver(env)]
