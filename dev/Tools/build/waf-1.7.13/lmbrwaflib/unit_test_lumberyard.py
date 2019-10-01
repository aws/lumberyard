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

from waflib import Errors

import unit_test
import lumberyard

import pytest
import json
import os


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
