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

from waflib.TaskGen import feature, after_method


@feature('c', 'cxx')
@after_method('apply_link')
@after_method('add_pch_to_dependencies')
def place_wall_first(self):
    for t in getattr(self, 'compiled_tasks', []):
        for flags_type in ('CFLAGS', 'CXXFLAGS'):
            flags = t.env[flags_type]
            try:
                wall_idx = flags.index('-Wall')
                if wall_idx > 0:
                    del flags[wall_idx]
                    flags.insert(0,'-Wall')
            except ValueError:
                continue
