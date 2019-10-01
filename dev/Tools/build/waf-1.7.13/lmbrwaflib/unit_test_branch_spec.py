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

import pytest

from waf_branch_spec import BINTEMP_FOLDER


class FakeGModule(object):
    pass


from waflib import Context

Context.g_module = FakeGModule


import branch_spec


class FakeContextBranchSpec(object):
    pass


class FakeBldNode(object):
    
    def __init__(self, name):
        self.name = name
        
    def __str__(self):
        return self.name
    
    def make_node(self, name):
        child = FakeBldNode(name)
        child.parent = self
        return child


@pytest.fixture
def fake_waf_context(fake_ctx_type):
    
    ctx = FakeContextBranchSpec()
    if fake_ctx_type == 'configure':
        setattr(ctx, 'path', FakeBldNode('dev'))

    elif fake_ctx_type == 'project_generator':
        setattr(ctx, 'bldnode', FakeBldNode(BINTEMP_FOLDER))
        
    elif fake_ctx_type == 'build':
        temp_dir_node = FakeBldNode(BINTEMP_FOLDER)
        setattr(ctx, 'bldnode', temp_dir_node.make_node('build_variant'))
        
    else:
        assert False
        
    return ctx


@pytest.mark.parametrize(
    "fake_ctx_type", [
        pytest.param('configure'),
        pytest.param('project_generator'),
        pytest.param('build')
    ]
)
def test_get_bintemp_folder_node(fake_waf_context, fake_ctx_type):
    
    result = branch_spec.get_bintemp_folder_node(fake_waf_context)
    assert str(result) == BINTEMP_FOLDER
