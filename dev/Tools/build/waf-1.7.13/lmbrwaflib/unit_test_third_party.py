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

import third_party
import unit_test
import pytest



@pytest.fixture
def fake_waf_context(tmpdir):
    
    if tmpdir:
        base_path = str(tmpdir.realpath())
    else:
        base_path = None
        
    fake_context = unit_test.FakeContext(base_path)

    fake_context.all_envs = {
        'platform_1_debug': {
            'INCLUDES_USELIB1' : ['foo'],
            'LIBPATH_USELIB2' : ['foo'],
            'LIB_USELIB3' : ['foo'],
            'STLIBPATH_USELIB4' : ['foo'],
            'STLIB_USELIB5' : ['foo'],
            'DEFINES_USELIB6' : ['foo'],
            'DEFINES_USELIB7' : [],
            'COPY_EXTRA_USELIB8' : ['foo'],
            'PDB_USELIB9' : ['foo'],
            'SHAREDLIBPATH_USELIB10' : ['foo'],
            'SHAREDLIB_USELIB11' : []
        }
    }
    
    def _mock_get_configuration_names() :
        return ['debug']
    fake_context.get_all_configuration_names = _mock_get_configuration_names

    yield fake_context


@pytest.mark.parametrize(
    "input_platform, input_uselib, expected",[
        pytest.param('platform_1', 'USELIB1', True, id="testSupportedPlatform1Uselib1"),
        pytest.param('platform_1', 'USELIB2', True, id="testSupportedPlatform1Uselib2"),
        pytest.param('platform_1', 'USELIB3', True, id="testSupportedPlatform1Uselib3"),
        pytest.param('platform_1', 'USELIB4', True, id="testSupportedPlatform1Uselib4"),
        pytest.param('platform_1', 'USELIB5', True, id="testSupportedPlatform1Uselib5"),
        pytest.param('platform_1', 'USELIB6', True, id="testSupportedPlatform1Uselib6"),
        pytest.param('platform_1', 'USELIB7', True, id="testSupportedPlatform1Uselib7"),
        pytest.param('platform_1', 'USELIB8', True, id="testSupportedPlatform1Uselib8"),
        pytest.param('platform_1', 'USELIB9', True, id="testSupportedPlatform1Uselib9"),
        pytest.param('platform_1', 'USELIB10', True, id="testSupportedPlatform1Uselib10"),
        pytest.param('platform_1', 'USELIB11', True, id="testSupportedPlatform1Uselib11"),
        pytest.param('platform_2', 'USELIB1', False, id="testUnSupportedPlatform1Uselib1InvalidPlatformName"),
        pytest.param('platform_1', 'USELIBXX', False, id="testUnSupportedPlatform1Uselib2UnsupportedUselibName")
    ])
def test_CheckPlatformUselibCompatibility_ValidInputPermutations_TrueOrFalse(fake_waf_context, input_platform, input_uselib, expected):
    fake_waf_context.env['PLATFORM'] = input_platform
    result = third_party.check_platform_uselib_compatibility(fake_waf_context, input_uselib)
    assert result == expected


def test_CheckPlatformUselibCompatibility_ProjectGenerator_None(fake_waf_context):
    fake_waf_context.env['PLATFORM'] = 'project_generator'
    result = third_party.check_platform_uselib_compatibility(fake_waf_context, 'USELIB1')
    assert result is None

