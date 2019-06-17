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

from waflib import Utils, Errors, Configure
from cry_utils import split_comma_delimited_string, get_waf_host_platform, read_file_list

import json
import utils
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
    "input_param,expected,enforce_uniqueness",[
        pytest.param("a,b,c,d,e,f,b,d,e,g", ['a', 'b', 'c', 'd', 'e', 'f', 'b', 'd', 'e', 'g'], False, id="NoTrimNoEnforceUniqueness"),
        pytest.param("a, b, c,d,e,f,b,d,e,g,a  ,b,c ", ['a', 'b', 'c', 'd', 'e', 'f', 'b', 'd', 'e', 'g', 'a', 'b', 'c'], False, id="TrimNoEnforceUniqueness"),
        pytest.param("a,b,c,d,e,f,b,d,e,g", ['a', 'b', 'c', 'd', 'e', 'f', 'g'], True, id="NoTrimEnforceUniqueness"),
        pytest.param("a, b, c,d,e,f,b,d,e,g,a  ,b,c ", ['a', 'b', 'c', 'd', 'e', 'f', 'g'], True, id="TrimEnforceUniqueness"),
        pytest.param(" ,  , ,", [], False, id="EmptyCommasEnforceUniqueness"),
        pytest.param(" ,  , ,", [], True,  id="EmptyCommasNoEnforceUniqueness"),
        pytest.param(" ", [], False, id="BlankStringEnforceUniqueness"),
        pytest.param(" ", [], True, id="BlankStringNoEnforceUniqueness"),
        pytest.param("", [], False, id="EmptyStringEnforceUniqueness"),
        pytest.param("", [], True, id="EmptyStringNoEnforceUniqueness"),
    ])
def test_SplitCommaDelimitedString_ValidInputPermutations_Success(input_param, expected, enforce_uniqueness):
    result = split_comma_delimited_string(input_str=input_param,
                                          enforce_uniqueness=enforce_uniqueness)
    assert result == expected


@pytest.fixture
def get_waf_host_platform_runner(mock_versioned_platform):
    
    def _make_get_unversioned_sys_platform():
        return mock_versioned_platform
    
    old_func = Utils.unversioned_sys_platform
    Utils.unversioned_sys_platform = _make_get_unversioned_sys_platform
    yield
    Utils.unversioned_sys_platform = old_func


@pytest.mark.parametrize(
    "mock_versioned_platform,expected_unversioned_platform",[
        pytest.param('win32', 'win_x64', id="get_waf_host_platform()=='win_x64'"),
        pytest.param('darwin', 'darwin_x64', id="get_waf_host_platform()=='darwin_x64'"),
        pytest.param('linux', 'linux_x64', id="get_waf_host_platform()=='linux_x64'"),
    ])
def test_GetWafHostPlatform_ValidSupportedPlatforms_Success(mock_versioned_platform, expected_unversioned_platform, fake_waf_context, get_waf_host_platform_runner):
    
    result = get_waf_host_platform(fake_waf_context)
    assert expected_unversioned_platform == result
    

@pytest.mark.parametrize(
    "mock_versioned_platform", [
        pytest.param('atari2600', id="get_waf_host_platform() [exception]")
    ]
)
def test_GetWafHostPlatform_UnsupportedPlatform_Exception(mock_versioned_platform, fake_waf_context, get_waf_host_platform_runner):
    with pytest.raises(Errors.WafError):
        get_waf_host_platform(fake_waf_context)


@pytest.mark.parametrize(
    "input_waf_file_list, sample_files, dynamic_globbing, expected_results", [
        
        pytest.param(   # Basic recursive for all files No Dynamic Globbing
                        {   # Input waf_file content
                            "none": {
                                "root": [
                                    "**/*"
                                ]
                            }
                        },
                        # Sample Data
                        ["a.cpp",
                         "b.cpp",
                         "includes/a.h",
                         "includes/b.h"],
                        # Enable dynamic globbing?
                        False,
                        # Expected result
                        {
                            'none': {
                                'root': [
                                    'a.cpp',
                                    'b.cpp',
                                    'test.waf_files'
                                ],
                                'includes': [
                                    'includes/a.h',
                                    'includes/b.h'
                                ]
                            }
                        },
                        # Id
                        id="BasicRecursiveForAllFiles"),
        
        pytest.param(   # Basic recursive for all files Dynamic Globbing
                        {
                            "none": {
                                "root": [
                                    "**/*"
                                ]
                            }
                        },
                        # Sample Data
                        ["a.cpp",
                         "b.cpp",
                         "includes/a.h",
                         "includes/b.h"],
                        # Enable dynamic globbing?
                        True,
                        # Expected result
                        {
                            'none': {
                                'root': [
                                    'a.cpp',
                                    'b.cpp',
                                    'test.waf_files'
                                ],
                                'includes': [
                                    'includes/a.h',
                                    'includes/b.h'
                                ]
                            }
                        },
                        # Id
                        id="BasicRecursiveForAllFilesDynamicGlobbing"),
        
        pytest.param(   # Recursive search for .cpp/.h only
                        {
                            "none": {
                                "root": [
                                    "**/*.cpp",
                                    "**/*.h"
                                ]
                            }
                        },
                        # Sample Data
                        ["a.cpp",
                         "b.cpp",
                         "a.ignore",
                         "includes/a.h",
                         "includes/b.h",
                         "ignore/b.ignore"],
                        # Enable dynamic globbing?
                        False,
                        # Expected result
                        {
                            'none': {
                                'root': [
                                    'a.cpp',
                                    'b.cpp'
                                ],
                                'includes': [
                                    'includes/a.h',
                                    'includes/b.h'
                                ]
                            }
                        },
                        # Id
                        id="RecursiveSearchForCppAndHOnly"),
        
        pytest.param(   # Search using advanced search pattern and rules
                        {
                            "none": {
                                "root": [
                                    {
                                        "pattern": "**/*",
                                        "excl": "*.waf_files"
                                    }
                                ]
                            }
                        },
                        # Sample Data
                        ["a.cpp",
                         "b.cpp",
                         "includes/a.h",
                         "includes/b.h"],
                        # Enable dynamic globbing?
                        False,
                        # Expected result
                        {
                            'none': {
                                'root': [
                                    'a.cpp',
                                    'b.cpp'
                                ],
                                'includes': [
                                    'includes/a.h',
                                    'includes/b.h'
                                ]
                            }
                        },
                        # Id
                        id="SimpleNestedLevelAllExcludeThroughCustomPattern"),
        pytest.param(   # SingleAndNestedPatterns
                        {
                            "none": {
                                "root": [
                                    "*.cpp",
                                    "*.h"
                                ],
                                "single": [
                                    "single/*.cpp",
                                    "single/*.h"
                                ],
                                "nested": [
                                    "nested/**/*.cpp",
                                    "nested/**/*.h"
                                ]
                            }
                        },
                        # Sample Data
                        ["a.cpp",
                         "b.cpp",
                         "ignore/a.h",
                         "ignore/b.h",
                         "single/s1_a.cpp",
                         "single/s1_b.cpp",
                         "single/ignore/s1_a.cpp",
                         "nested/n1_a.cpp",
                         "nested/n1_b.cpp",
                         "nested/include/a.h",
                         "nested/include/b.h",
                         "ignore/ignore_a.cpp",
                         "ignore/ignore_b.cpp"],
                        # Enable dynamic globbing?
                        False,
                        # Expected result
                        {
                            'none': {
                                'nested/nested': [
                                    'nested/n1_a.cpp',
                                    'nested/n1_b.cpp'
                                ],
                                'nested/nested/include': [
                                    'nested/include/a.h',
                                    'nested/include/b.h'
                                ],
                                'single': [
                                    'single/s1_a.cpp',
                                    'single/s1_b.cpp'
                                ],
                                'root': [
                                    'a.cpp',
                                    'b.cpp'
                                ]
                            }
                        },
                        # Id
                        id="SingleAndNestedLevelsSpecifiedTypes")
    ]
)
def test_ReadFileList_SimpleGlobPatternNonDynamic_Success(fake_waf_context, mock_globbing_files, tmpdir, input_waf_file_list, sample_files, dynamic_globbing, expected_results):
    
    # Arrange
    def _mock_is_option_true(option):
        assert option == 'enable_dynamic_file_globbing'
        return dynamic_globbing
    fake_waf_context.is_option_true = _mock_is_option_true

    # Act
    try:
        old_config_context = Configure.ConfigurationContext
        Configure.ConfigurationContext = unit_test.FakeContext
        result = read_file_list(fake_waf_context, mock_globbing_files)
    finally:
        Configure.ConfigurationContext = old_config_context
    
    # Assert
    bintemp_path = fake_waf_context.bintemp_node.abspath()
    src_code_path = fake_waf_context.path
    expected_cached_waf_files = os.path.join(bintemp_path, src_code_path.name, mock_globbing_files)
    if not dynamic_globbing:
        assert os.path.isfile(expected_cached_waf_files)
        cached_waf_file_result = utils.parse_json_file(expected_cached_waf_files)
        assert cached_waf_file_result == expected_results
    else:
        assert not os.path.isfile(expected_cached_waf_files)
    
    assert result == expected_results


@pytest.fixture()
def mock_globbing_files(fake_waf_context, tmpdir, input_waf_file_list, sample_files):
    
    # We need the path relative to the tmpdir in order to write the testing temp data
    path_abs = fake_waf_context.path.abspath()
    tmpdir_path = str(tmpdir.realpath())
    path_rel = os.path.relpath(path_abs, tmpdir_path)
    
    # Create the temp 'test.waf_files'
    test_file_waffiles = "test.waf_files"
    file_list_path = os.path.normpath(os.path.join(path_rel, test_file_waffiles))
    tmpdir.ensure(file_list_path)
    waf_file_pypath = tmpdir.join(file_list_path)
    json_file_content = json.dumps(input_waf_file_list,
                                   sort_keys=True,
                                   separators=(',', ': '),
                                   indent=4)
    waf_file_pypath.write(json_file_content)
    
    # Create the sample files relative to the 'path_rel' in the temp folder, which should be where the test waf_files
    # file will reside (and all files are supposed to be relative)
    for sample_file in sample_files:
        sample_file_target_path = os.path.normpath(os.path.join(path_rel, sample_file))
        
        tmpdir.ensure(sample_file_target_path)
        
        sample_file_pypath = tmpdir.join(sample_file_target_path)
        sample_file_pypath.write("self.path = {}".format(sample_file_target_path))
        
    yield test_file_waffiles

    tmpdir.remove(ignore_errors=True)

