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

# System Imports
import json
import os
import pytest

# waflib imports
from waflib import Errors

# lmbrwaflib imports
from lmbrwaflib import unit_test
from lmbrwaflib import lumberyard



BASIC_ENGINE_JSON = {
        'FileVersion': 1,
        'LumberyardVersion': '0.0.0.0',
        'LumberyardCopyrightYear': 2019
    }


@pytest.mark.parametrize(
    "engine_json, expected_error", [
        pytest.param(None,              Errors.WafError, id="no_engine_json"),
        pytest.param(BASIC_ENGINE_JSON, None,            id="internal_engine_json"),
    ]
)
def test_get_engine_node_internal(tmpdir, engine_json, expected_error):
    
    if engine_json:
        engine_json_content = json.dumps(engine_json,
                                         sort_keys=True,
                                         separators=(',', ': '),
                                         indent=4)
        tmpdir.ensure('dev', dir=True)
        tmpdir.join('dev/engine.json').write(engine_json_content)
    
    try:
        fake_context = unit_test.FakeContext(str(tmpdir.realpath()), False)
        fake_context.path = fake_context.srcnode
        lumberyard.get_engine_node(fake_context)
    except expected_error:
        pass


@pytest.mark.parametrize(
    "external_engine_json, ext_engine_subpath, expected_error", [
        pytest.param(None,              '../external1', Errors.WafError, id="Invalid_External_engine_path"),
        pytest.param(BASIC_ENGINE_JSON, '../external1', None,            id="Valid_External_engine_rel_path"),
        pytest.param(BASIC_ENGINE_JSON, 'external1',    None,            id="Valid_External_engine_abs_path")
    ]
)
def test_get_engine_node_external(tmpdir, external_engine_json, ext_engine_subpath, expected_error):

    tmp_working_path = str(tmpdir.realpath())
    c = os.path.join(tmp_working_path, os.path.normpath(ext_engine_subpath))
    b = os.path.normpath(c)
    tmp_working_engine_path = os.path.realpath(b)

    if ext_engine_subpath.startswith('..'):
        json_external_engine_path = ext_engine_subpath
    else:
        json_external_engine_path = tmp_working_engine_path
        
    engine_json = {
        'ExternalEnginePath':   json_external_engine_path,
        'FileVersion': 1,
        'LumberyardVersion': '0.0.0.0',
        'LumberyardCopyrightYear': 2019
    }
    
    rel_path = os.path.relpath(tmp_working_engine_path, tmp_working_path)
    tmpdir.ensure(rel_path, dir=True)
    
    engine_json_content = json.dumps(engine_json,
                                     sort_keys=True,
                                     separators=(',', ': '),
                                     indent=4)
    tmpdir.ensure('dev', dir=True)
    tmpdir.join('dev/engine.json').write(engine_json_content)
    
    if external_engine_json:
        external_engine_json_content = json.dumps(external_engine_json,
                                                  sort_keys=True,
                                                  separators=(',', ': '),
                                                  indent=4)
        if rel_path.startswith('..'):
            rel_path = rel_path[3:]
            
        tmpdir.ensure(rel_path, dir=True)
        tmpdir.join('{}/engine.json'.format(rel_path)).write(external_engine_json_content)
    try:
        fake_context = unit_test.FakeContext(tmp_working_path, False)
        fake_context.path = fake_context.srcnode
        lumberyard.get_engine_node(fake_context)
    except expected_error:
        pass


def test_get_all_eligible_use_keywords():
    
    class MockPlatformSettings(object):
        def __init__(self):
            self.aliases = ['alias_foo']
    
    class MockContext(object):
        
        def get_all_platform_names(self):
            return ['platform_foo']
        
        def get_platform_settings(self,platform_name):
            assert platform_name == 'platform_foo'
            return MockPlatformSettings()
        
    mockCtx = MockContext()
    
    related_keywords = lumberyard.get_all_eligible_use_keywords(mockCtx)
    
    expected_use_keywords = ['use', 'test_use', 'test_all_use', 'platform_foo_use', 'alias_foo_use']
    assert len(related_keywords) == len(expected_use_keywords)
    for expected_use in expected_use_keywords:
        assert expected_use in related_keywords


@pytest.mark.parametrize(
    "engine_root_version, experimental_string, expected", [
        pytest.param('0.0.0.0', 'False', True),
        pytest.param('0.0.0.0', 'True', True),
        pytest.param('0.0.0.1', 'False', False),
        pytest.param('0.0.0.1', 'True', True)
    ]
)
def test_should_build_experimental_targets(engine_root_version, experimental_string, expected):
    
    class FakeOptions(object):
        def __init__(self, experimental_string):
            self.enable_experimental_features = experimental_string
    
    class FakeExperimentContext(object):
        def __init__(self, engine_root_version, experimental_string):
            
            self.engine_root_version = engine_root_version
            self.options = FakeOptions(experimental_string)
            
    fake_context = FakeExperimentContext(engine_root_version, experimental_string)
    
    result = lumberyard.should_build_experimental_targets(fake_context)
    
    assert result == expected
