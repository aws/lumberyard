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

import settings_manager

TEST_DICT = {
    'has_test_configs': True,
    'has_server_configs': True,
    'output_folder_ext_test_a': 'ext_test_a',
    'output_folder_ext_test_b': 'ext_test_b',
    'output_folder_ext_test_c': 'ext_test_c',
    'output_folder_ext_test_d': 'ext_test_d',
    'output_folder_ext_test_e': 'ext_test_e',
    'output_folder_ext_test_f': 'ext_test_f',
}


MOCK_LUMBERYARD_SETTINGS = settings_manager.Settings()

# We have to stub out LUMBERYARD_SETTINGS.get_settings_value since its done during module loading
def _stub_get_settings_value(key):
    return TEST_DICT[key]
MOCK_LUMBERYARD_SETTINGS.get_settings_value = _stub_get_settings_value

def _stub_get_all_platform_settings():
    return []
MOCK_LUMBERYARD_SETTINGS.get_all_platform_settings = _stub_get_all_platform_settings


import build_configurations


class MockConfigSettings(object):
    def __init__(self, name, has_test, is_monolithic):
        self.name = name
        self.is_monolithic = is_monolithic
        self.has_test = has_test

    def config_name(self):
        return self.name
    
    def build_config_name(self, is_test, is_server):
        config_name = self.name
        if is_test:
            config_name += '_test'
        if is_server:
            config_name += '_dedicated'
        return config_name

        
@pytest.mark.parametrize(
    "enable_test, enable_server, platform, enabled, has_server, has_tests, is_monolithic, output_folder, aliases, attributes, needs_java, platform_env_dict, expected_monotype_map", [
        #            'enable_test', 'enable_server', 'platform', 'enabled', 'has_server', 'has_tests', 'is_monolithic', 'output_folder', 'aliases', 'attributes', 'needs_java', 'platform_env_dict')
        pytest.param(False,  False, 'platform1', False, False, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(False, False, 'platform2', True, False, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),

        pytest.param(False, False, 'platform3', True, False, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(False, False, 'platform4', True, True,  False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(False, False, 'platform5', True, False, True,  False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(False, False, 'platform6', True, True,  True,  False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        
        pytest.param(False, True,  'platform7', True, False, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(False, True,  'platform8', True, True,  False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(False, True,  'platform9', True, False, True,  False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(False, True,  'platformA', True, True,  True,  False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        
        pytest.param(True, False,  'platformB', True, False, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(True, False,  'platformC', True, True, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(True, False,  'platformD', True, False, True, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(True, False,  'platformE', True, True, True, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        
        pytest.param(True, True, 'platformB', True, False, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(True, True, 'platformC', True, True, False, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(True, True, 'platformD', True, False, True, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(True, True, 'platformE', True, True, True, False, 'output_folder', {'aliases'}, {}, False, {}, {}),
        pytest.param(True, True, 'platformF', True, True, True, False, 'output_folder', {'aliases'}, {}, False, {}, {'debug': False, 'profile': False, 'release': True}),
        pytest.param(True, True, 'platformF', True, True, True, [], 'output_folder', {'aliases'}, {}, False, {}, {'debug': False, 'profile': False, 'release': True}),
        pytest.param(True, True, 'platformG', True, True, True, True, 'output_folder', {'aliases'}, {}, False, {}, {'debug': True, 'profile': True, 'release': True}),
        pytest.param(True, True, 'platformG', True, True, True, 'debug', 'output_folder', {'aliases'}, {}, False, {}, {'debug': True, 'profile': False, 'release': True}),
        pytest.param(True, True, 'platformG', True, True, True, ['debug'], 'output_folder', {'aliases'}, {}, False, {}, {'debug': True, 'profile': False, 'release': True}),
        pytest.param(True, True, 'platformG', True, True, True, 'profile', 'output_folder', {'aliases'}, {}, False, {}, {'debug': False, 'profile': True, 'release': True}),
        pytest.param(True, True, 'platformG', True, True, True, ['profile'], 'output_folder', {'aliases'}, {}, False, {}, {'debug': False, 'profile': True, 'release': True}),
        pytest.param(True, True, 'platformG', True, True, True, ['debug', 'profile'], 'output_folder', {'aliases'}, {}, False, {}, {'debug': True, 'profile': True, 'release': True}),
    ]
)
def test_PlatformDetail_Init_Success(enable_test, enable_server, platform, enabled, has_server, has_tests, is_monolithic, output_folder, aliases, attributes, needs_java, platform_env_dict, expected_monotype_map):
    
    old_enable_test_configurations = build_configurations.ENABLE_TEST_CONFIGURATIONS
    old_enable_server = build_configurations.ENABLE_SERVER
    old_lumberyard_settings = settings_manager.LUMBERYARD_SETTINGS
    settings_manager.LUMBERYARD_SETTINGS = MOCK_LUMBERYARD_SETTINGS
    try:
        build_configurations.ENABLE_TEST_CONFIGURATIONS = enable_test
        build_configurations.ENABLE_SERVER = enable_server

        DEBUG = MockConfigSettings('debug', has_tests, False)
        PROFILE = MockConfigSettings('profile', has_tests, False)
        RELEASE = MockConfigSettings('release', False, True)

        def _mock_get_build_configuration_settings():
            return [DEBUG, PROFILE, RELEASE]
        MOCK_LUMBERYARD_SETTINGS.get_build_configuration_settings = _mock_get_build_configuration_settings
        
        test = build_configurations.PlatformDetail(platform, enabled, has_server, has_tests, is_monolithic, output_folder, aliases, attributes, needs_java, platform_env_dict)
        
        assert enabled == test.enabled
        assert aliases == test.aliases
        assert has_server == test.has_server
        assert has_tests == test.has_tests
        assert output_folder == test.output_folder
        assert platform == test.platform
        assert platform == test.third_party_platform_key
        
        assert 'debug' in test.platform_configs
        assert 'profile' in test.platform_configs
        assert 'release' in test.platform_configs
        
        has_debug_test = 'debug_test' in test.platform_configs
        has_profile_test = 'profile_test' in test.platform_configs
        has_debug_server = 'debug_dedicated' in test.platform_configs
        has_profile_server = 'profile_dedicated' in test.platform_configs
        has_debug_test_server = 'debug_test_dedicated' in test.platform_configs
        has_profile_test_server = 'profile_test_dedicated' in test.platform_configs
        
        if not enable_test and not enable_server:
            assert not has_debug_test
            assert not has_profile_test
            assert not has_debug_server
            assert not has_profile_server
            assert not has_debug_test_server
            assert not has_profile_test_server

        elif enable_test and not enable_server:
            if not has_tests:
                assert not has_debug_test
                assert not has_profile_test
                assert not has_debug_server
                assert not has_profile_server
                assert not has_debug_test_server
                assert not has_profile_test_server
            elif has_tests:
                assert has_debug_test
                assert has_profile_test
                assert not has_debug_server
                assert not has_profile_server
                assert not has_debug_test_server
                assert not has_profile_test_server
        elif not enable_test and enable_server:
            if has_server:
                assert not has_debug_test
                assert not has_profile_test
                assert has_debug_server
                assert has_profile_server
                assert not has_debug_test_server
                assert not has_profile_test_server
            elif not has_server:
                assert not has_debug_test
                assert not has_profile_test
                assert not has_debug_server
                assert not has_profile_server
                assert not has_debug_test_server
                assert not has_profile_test_server
        elif enable_test and enable_server:
            if not has_tests and not has_server:
                assert not has_debug_test
                assert not has_profile_test
                assert not has_debug_server
                assert not has_profile_server
                assert not has_debug_test_server
                assert not has_profile_test_server
            elif not has_tests and has_server:
                assert not has_debug_test
                assert not has_profile_test
                assert has_debug_server
                assert has_profile_server
                assert not has_debug_test_server
                assert not has_profile_test_server
            elif has_tests and not has_server:
                assert has_debug_test
                assert has_profile_test
                assert not has_debug_server
                assert not has_profile_server
                assert not has_debug_test_server
                assert not has_profile_test_server
            elif has_tests and has_server:
                assert has_debug_test
                assert has_profile_test
                assert has_debug_server
                assert has_profile_server
                assert has_debug_test_server
                assert has_profile_test_server
                
            if expected_monotype_map:
                for expected_config, expected_monolithic in expected_monotype_map.items():
                    assert test.platform_configs[expected_config].is_monolithic == expected_monolithic
    finally:
        build_configurations.ENABLE_TEST_CONFIGURATIONS = old_enable_test_configurations
        build_configurations.ENABLE_SERVER = old_enable_server
        settings_manager.LUMBERYARD_SETTINGS = old_lumberyard_settings


class MockPlatformSettings(object):
    
    def __init__(self, aliases, platform, is_monolithic, enabled=True, has_server=True, has_test=True):
        self.aliases = aliases
        self.platform = platform
        self.is_monolithic = is_monolithic
        self.enabled = enabled
        self.has_server = has_server
        self.has_test = has_test
        self.attributes = {}
        self.needs_java = False
        self.env_dict = {}


class MockLumberyardSettings(object):
    
    def __init__(self):
        self.mock_platform_settings = []
        self.settings_map = {}
    
    def get_all_platform_settings(self):
        return self.mock_platform_settings
    
    def add_mock_platform(self, platform, is_monolithic, alias_str):
        output_folder_key = 'out_folder_{}'.format(platform)
        self.settings_map[output_folder_key] = 'output_{}'.format(platform)
        
        mock_platform_setting = MockPlatformSettings(aliases=alias_str.split(','),
                                                     platform=platform,
                                                     is_monolithic=is_monolithic)
        self.mock_platform_settings.append(mock_platform_setting)
    
    def get_settings_value(self, settings):
        return self.settings_map[settings]
    
    def get_build_configuration_settings(self):
        return []


def test_ReadPlatformsForHost_CheckMonolithicBuildOverride():
    old_lumberyard_settings = settings_manager.LUMBERYARD_SETTINGS
    try:
        mock_settings = MockLumberyardSettings()
        # Setup 3 platforms, foo and bar are normal platforms, and 'ret' is monolithic only
        mock_settings.add_mock_platform(platform='foo', is_monolithic=False, alias_str='fooo,phoo')
        mock_settings.add_mock_platform(platform='bar', is_monolithic=False, alias_str='bahr,bbar')
        mock_settings.add_mock_platform(platform='ret', is_monolithic=True, alias_str='rhet')
        
        # Attempt to override the monolithic only value for 'foo' to True (always monolithic)
        mock_settings.settings_map['foo_build_monolithic'] = True
        # Attempt to override the monolithic only value for 'ret' to False. (default to the original value)
        mock_settings.settings_map['ret_build_monolithic'] = False
        
        settings_manager.LUMBERYARD_SETTINGS = mock_settings
        
        validated_platforms_for_host, alias_to_platforms_map = build_configurations._read_platforms_for_host()
        
        assert validated_platforms_for_host['foo'].is_monolithic == True  # Expect the monolithic platform setting to be true from the override setting
        assert validated_platforms_for_host['bar'].is_monolithic == False  # Expect the monolithic platform setting to be false from the default setting
        assert validated_platforms_for_host['ret'].is_monolithic == True  # Expect the monolithic platform setting to be false from the default setting even though we try to override it
    
    finally:
        settings_manager.LUMBERYARD_SETTINGS = old_lumberyard_settings
