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

import sys
sys.path += '.'

from waflib import Utils, Errors, Node

import utils

import os


class FakeContext(object):
    
    def __init__(self, base_path, generate_engine_json=True):
        
        if base_path:
            # binds the context to the nodes in use to avoid a context singleton
            class node_class(Node.Node):
                pass
            self.node_class = node_class
            self.node_class.__module__ = "waflib.Node"
            self.node_class.__name__ = "Nod3"
            self.node_class.ctx = self
    
            self.root = self.node_class('', None)

            self.base_node = self.root.make_node(base_path)
            self.srcnode = self.base_node.make_node('dev')
            self.bintemp_node = self.srcnode.make_node('BinTemp')
            self.path = self.srcnode.make_node('Code')
            self.bldnode = self.bintemp_node
            if generate_engine_json:
                default_engine_json = {
                        "FileVersion": 1,
                        "LumberyardVersion": "0.0.0.0",
                        "LumberyardCopyrightYear": 2019
                }
                utils.write_json_file(default_engine_json, os.path.join(base_path, 'engine.json'))
        
        self.env = {}
        self.project_overrides = {}
        self.file_overrides = {}
    
    def get_bintemp_folder_node(self):
        return self.bintemp_node
    
    def get_project_overrides(self, target):
        return self.project_overrides

    def get_file_overrides(self, target):
        return self.file_overrides
    
    def fatal(self, msg):
        raise Errors.WafError

    def parse_json_file(self, node):
        return utils.parse_json_file(node.abspath())
