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

from waflib import Utils, Errors, Node
from utils import parse_json_file


class FakeContext(object):
    
    def __init__(self, base_path):
        
        if base_path:
            
            self.root = Node.Node('', None)
            self.base_node = self.root.make_node(base_path)
            self.srcnode = self.base_node.make_node('dev')
            self.bintemp_node = self.srcnode.make_node('BinTemp')
            self.path = self.srcnode.make_node('Code')
        
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
        return parse_json_file(node.abspath())
