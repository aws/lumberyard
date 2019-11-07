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
"""
Usage:

def options(opt):
    opt.load('module_info')

$ waf info
"""
from waflib import Build, Errors, Logs, Node, TaskGen, Utils

from pprint import pprint, pformat
from collections import OrderedDict

import json
import os
import re

def to_list(value):
    if isinstance(value, set):
        return list(value)
    elif isinstance(value, list):
        return value[:]
    else:
        return [value]

class ModuleInfoContext(Build.BuildContext):
    cmd = 'info'

    def execute(self):
        # restore the environments
        self.restore()
        if not self.all_envs:
            self.load_envs()

        self.load_user_settings()
        Logs.info("[WAF] Executing 'info' in '{}'".format(self.variant_dir))
        self.recurse([ self.run_dir ])

        output_dir_node = self.path.make_node(['BinTemp', 'module_info'])
        output_dir_node.mkdir()
        output_dir = output_dir_node.abspath()

        third_party_artifacts = []
        for group in self.groups:
            for task_generator in group:
                if not isinstance(task_generator, TaskGen.task_gen):
                    continue

                target_name = task_generator.target or task_generator.name

                if self.options.targets and target_name not in self.options.targets:
                    continue

                task_generator.post()

                path = task_generator.path.abspath()
                third_party_artifacts += getattr(task_generator.env, 'THIRD_PARTY_USELIBS', [])

                includes = OrderedDict()

                # Module includes
                for entry in to_list(getattr(task_generator, 'includes', [])):
                    if isinstance(entry, Node.Node):
                        entry = entry.abspath()

                    if not os.path.isabs(entry):
                        entry = os.path.join(path, entry)

                    entry = os.path.normpath(entry)
                    if ('\\bintemp\\' not in entry.lower()) and ('/bintemp/' not in entry.lower()): # Ignore all generated stuff!
                        includes[entry] = None

                # 3rdParty module includes
                dependencies = to_list(getattr(task_generator, 'use', [])) + to_list(getattr(task_generator, 'uselib', []))
                for dependency in sorted(set(dependencies)):
                    includes_key = 'INCLUDES_{}'.format(dependency)

                    for env in self.all_envs:
                        for entry in self.all_envs[env][includes_key]:
                            if isinstance(entry, Node.Node):
                                entry = entry.abspath()

                            entry = os.path.normpath(entry)
                            if ('\\bintemp\\' not in entry.lower()) and ('/bintemp/' not in entry.lower()): # Ignore all generated stuff!
                                includes[entry] = None

                # System includes
                for env in self.all_envs:
                    for entry in self.all_envs[env]['INCLUDES']:
                        if isinstance(entry, Node.Node):
                            entry = entry.abspath()

                        entry = os.path.normpath(entry)
                        includes[entry] = None

                sources = set()
                for entry in to_list(getattr(task_generator, 'source', [])):
                    if isinstance(entry, Node.Node):
                        entry = entry.abspath()

                    if not os.path.isabs(entry):
                      entry = os.path.join(path, entry)

                    entry = os.path.normpath(entry)
                    if ('\\bintemp\\' not in entry.lower()) and ('/bintemp/' not in entry.lower()): # Ignore all generated stuff!
                        sources.add(entry)

                module = OrderedDict([
                  ('name', target_name),
                  ('location', path),
                  ('includes', includes.keys()),
                  ('sources', sorted(sources)),
                  ('dependencies', sorted(dependencies))
                ])

                output_file = os.path.join(output_dir, target_name + '.json')
                with open(output_file, 'wt') as ostrm:
                    json.dump(module, ostrm, indent=2, separators=(',', ': '))
                    ostrm.flush()

        CONFIGURED_3RD_PARTY_USELIBS = self.read_and_mark_3rd_party_libs()
        SDKs = self.tp.content.get('SDKs')
        third_party_root = self.tp.content.get('3rdPartyRoot')

        def _find_sdk_location(config):
            names = [config['name']] if isinstance(config['name'], str) else config['name']
            for name in names:
                if name in SDKs:
                    relpath = SDKs[name]['base_source']
                    return os.path.join(third_party_root, relpath)

            source = config['source']
            if '@3P' in source:
                sdk_alias = source[source.find(':') + 1 : -1]
                if sdk_alias in SDKs:
                    relpath = SDKs[sdk_alias]['base_source']
                    return os.path.join(third_party_root, relpath)

            if '@ROOT@' in source:
                relpath = source[source.find('@ROOT@') + 6:]
                return os.path.join(self.engine_path, relpath)

            return None

        for artifact_name in set(third_party_artifacts):
            includes_key = 'INCLUDES_{}'.format(artifact_name)

            includes = OrderedDict()
            for env in self.all_envs:
                for path in self.all_envs[env][includes_key]:
                    includes[path] = None

            lib_config_file, _ = CONFIGURED_3RD_PARTY_USELIBS.get(artifact_name, (None, None))
            config, _, _ = self.get_3rd_party_config_record(lib_config_file) if lib_config_file else (None, None, None)
            location = _find_sdk_location(config) if config else None

            if not location:
                location = third_party_root
                Logs.error('SDK location not found for: ' + artifact_name)

            module = OrderedDict([
              ('name', artifact_name),
              ('location', os.path.normpath(location)),
              ('includes', includes.keys())
            ])

            output_file = os.path.join(output_dir, artifact_name + '.json')
            with open(output_file, 'wt') as ostrm:
                json.dump(module, ostrm, indent=2, separators=(',', ': '))
                ostrm.flush()
