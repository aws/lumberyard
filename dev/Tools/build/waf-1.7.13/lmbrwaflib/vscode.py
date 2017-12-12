#!/usr/bin/env python
# encoding: utf-8

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

from waflib import Logs, Utils, Errors, Build
from waflib.Context import BOTH
import os, string, json

tasks_json_contents = {
    "version": "0.1.0",
    "command": "lmbr_waf",
    "isShellCommand": True,
    "args": ["build_{platform}_{arch}_{configuration}"],
    "showOutput": "always"
}

settings_json_contents = {
    "files.exclude": {
        "Bin*": True,
        "Cache": True,
        "AssetProcessorTemp": True,
        "Solutions": True
    },
    "files.associations": {
        "wscript": "python",
        "*.slice": "xml"
    },
}
        
def write_obj(node, obj):
    node.write(json.dumps(obj, indent=4, separators=(',', ': ')))

class GenerateVSCodeWorkspace(Build.BuildContext):
    cmd = 'vscode'
    fun = 'build'

    def execute(self):
        # restore the environments
        self.restore()
        if not self.all_envs:
            self.load_envs()

        self.load_user_settings()
        
        self.generate_vscode_project()
    
    def generate_tasks_json(self, vscode_dir):
        global tasks_json_contents
        tasks_json = vscode_dir.make_node('tasks.json')
        
        host = Utils.unversioned_sys_platform()
        if host not in ('win_x64', 'win32'):
            # lmbr_waf -> lmbr_waf.sh
            tasks_json_contents["command"] = "./" + tasks_json_contents["command"] + ".sh"

        platform = host
        arch = "x64"
        if host in ('win_x64', 'win32'):
            vs = {
                "14" : 'vs2015',
                "12" : 'vs2013'
            }
            platform = 'win'
            arch = "x64_" + vs[self.options.msvs_version]
        variant = {
            "{platform}": platform,
            "{arch}": arch,
            "{configuration}": "debug"
        }

        args = tasks_json_contents["args"][0]
        for key, val in variant.iteritems():
            args = string.replace(args, key, val)
        tasks_json_contents["args"][0] = args
        write_obj(tasks_json, tasks_json_contents)
    
    def generate_settings_json(self, vscode_dir):
        global settings_json_contents
        settings_json = vscode_dir.make_node('settings.json')
        write_obj(settings_json, settings_json_contents)

    def generate_vscode_project(self):
        # ensure that the .vscode dir exists
        vscode_dir = self.srcnode.make_node('.vscode')
        vscode_dir.mkdir()

        self.generate_tasks_json(vscode_dir)
        self.generate_settings_json(vscode_dir)
