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

import utils
import incredibuild
import unit_test
import pytest
import os


@pytest.fixture
def fake_waf_context(tmpdir):
    
    if tmpdir:
        base_path = str(tmpdir.realpath())
    else:
        base_path = None
        
    yield unit_test.FakeContext(base_path)


@pytest.mark.parametrize(
    "input_tool_elements,expected_file_hash", [
        pytest.param([[]], "d0a710f30220392db25a22f5a69faac0"),
        pytest.param([[
                          '<Tool Filename="value1"/>']
                      ], "9c3d9373d224d3b9024f15e7e2a60afb"),
        pytest.param([[
                          '<Tool Filename="value1"/>',
                          '<Tool Filename="value2"/>'
                      ]], "344a67cc353c1dd16dda6cd6a6cad446"),
        pytest.param([[
                          '<Tool Filename="value1"/>'
                      ],
                      [
                          '<Tool Filename="value1"/>'
                      ]], "ad809694895f173b2b2a054c81f38663"),
    ])
def test_GenerateIbProfile_Permutations_Success(tmpdir, fake_waf_context, input_tool_elements, expected_file_hash):
    
    def _mock_generator_ib_profile_tool_elements():
        return input_tool_elements
    fake_waf_context.generate_ib_profile_tool_elements = _mock_generator_ib_profile_tool_elements
    tmpdir.ensure('dev/BinTemp', dir=True)
    
    incredibuild.generate_ib_profile_xml(fake_waf_context)
    
    result_profile_xml_target = os.path.join(fake_waf_context.get_bintemp_folder_node().abspath(), 'profile.xml')
    actual_hash = utils.calculate_file_hash(result_profile_xml_target)
    assert actual_hash == expected_file_hash


def test_GenerateIbProfile_NoOverwrite_Success(fake_waf_context):
    
    original_os_path_isfile = os.path.isfile
    original_calculate_string_hash = utils.calculate_string_hash
    original_calculate_file_hash = utils.calculate_file_hash
    try:
        def _mock_generator_ib_profile_tool_elements():
            return [[]]
        fake_waf_context.generate_ib_profile_tool_elements = _mock_generator_ib_profile_tool_elements
        
        def _mock_is_file(path):
            return True
        os.path.isfile = _mock_is_file
        
        def _mock_calculate_string_hash(content):
            return "HASH"
        utils.calculate_string_hash = _mock_calculate_string_hash
        
        def _mock_calculate_file_hash(filepath):
            return "HASH"
        utils.calculate_file_hash = _mock_calculate_file_hash
        
        incredibuild.generate_ib_profile_xml(fake_waf_context)
        
        result_profile_xml_target = os.path.join(fake_waf_context.get_bintemp_folder_node().abspath(), 'profile.xml')
    finally:
        os.path.isfile = original_os_path_isfile
        utils.calculate_string_hash = original_calculate_string_hash
        utils.calculate_file_hash = original_calculate_file_hash
        
    assert not os.path.isfile(result_profile_xml_target)
