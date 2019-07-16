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
import pytest, sys
from waflib import Logs, Errors, Utils
from waflib.Configure import ConfigurationContext
from waflib.Build import BuildContext

import settings_manager
import utils
import json
import os
import copy

class MockSettingsManagerConfigureContext(ConfigurationContext):
    """ Mock context class based on ConfigurationContext"""
    result_by_attribute = {}
    def __init__(self, **kw):
        super(ConfigurationContext, self).__init__(**kw)


class MockSettingsManagerBuildContext(BuildContext):
    """ Mock context class based on BuildContext"""
    result_by_attribute = {}
    def __init__(self, **kw):
        super(BuildContext, self).__init__(**kw)


@pytest.fixture()
def test_settings_manager_context_for_override_report(is_configure_context, is_option_name, is_option_value, override_settings_attributes):
    
    if is_configure_context:
        test_context = MockSettingsManagerConfigureContext(run_dir=sys.executable)
    else:
        test_context = MockSettingsManagerBuildContext(run_dir=sys.executable)
        setattr(test_context,'cmd', 'build_unit_test')
        
    def _stub_check_is_option_true(option_name):
        if not is_option_name:
            return True
        else:
            return is_option_value
        
    def _stub_override_settings_report(is_configure, is_build, attribute, default_value, settings_value):
        is_value_overridden =  default_value != settings_value
        test_context.result_by_attribute[attribute] = (is_configure, is_build, is_value_overridden)

    setattr(test_context, 'is_option_true', _stub_check_is_option_true)

    if override_settings_attributes:
        for override_settings_attribute in override_settings_attributes:
            report_settings_override_func_name = 'report_settings_{}'.format(override_settings_attribute)
            setattr(test_context, report_settings_override_func_name, _stub_override_settings_report)
            
    return test_context



@pytest.fixture()
def mocked_lumberyard_settings(test_default_settings_map, test_settings_map):
    
    original_default_settings_map = settings_manager.LUMBERYARD_SETTINGS.default_settings_map
    original_settings_map = settings_manager.LUMBERYARD_SETTINGS.settings_map

    settings_manager.LUMBERYARD_SETTINGS.settings_map = test_settings_map
    settings_manager.LUMBERYARD_SETTINGS.default_settings_map = test_default_settings_map
    
    yield

    settings_manager.LUMBERYARD_SETTINGS.settings_map = original_settings_map
    settings_manager.LUMBERYARD_SETTINGS.default_settings_map = original_default_settings_map

    
RECURSIVE_OPT = 'internal_dont_check_recursive_execution'


@pytest.mark.parametrize(
    "is_configure_context, is_option_name, is_option_value, override_settings_attributes, test_default_settings_map, test_settings_map, expected_result_map", [
        
        pytest.param(True, RECURSIVE_OPT, True, [], {}, {}, {}, id="ReportSettingsSkipRecursiveExecution"),
        
        pytest.param(True, RECURSIVE_OPT, False,
                     [],
                     {'attr': 'default'},
                     {'attr': 'default'},
                     {},
                     id="ReportSettingsConfigureContextNoOverrideNoReporter"),
        
        pytest.param(True, RECURSIVE_OPT, False,
                     ['attr1'],
                     {'attr1': 'default', 'attr2': 'default2'},
                     {'attr1': 'default', 'attr2': 'default2'},
                     {'attr1': (True, False, False)},
                     id="ReportSettingsConfigureContextNoOverrideWithSomeReporters"),
        
        pytest.param(True, RECURSIVE_OPT, False,
                     ['attr1', 'attr2'],
                     {'attr1': 'default', 'attr2': 'default2'},
                     {'attr1': 'default', 'attr2': 'default2'},
                     {'attr1': (True, False, False),
                      'attr2': (True, False, False)},
                     id="ReportSettingsConfigureContextNoOverrideWithAllReporter"),
        
        pytest.param(True, RECURSIVE_OPT, False,
                     ['attr1', 'attr2'],
                     {'attr1': 'default', 'attr2': 'default2'},
                     {'attr1': 'override', 'attr2': 'override2'},
                     {'attr1': (True, False, True),
                      'attr2': (True, False, True)},
                     id="ReportSettingsConfigureContextOverrideWithAllReporter"),

        pytest.param(False, RECURSIVE_OPT, False,
                     ['attr1', 'attr2'],
                     {'attr1': 'default', 'attr2': 'default2'},
                     {'attr1': 'default', 'attr2': 'default2'},
                     {'attr1': (False, True, False),
                      'attr2': (False, True, False)},
                     id="ReportSettingsBuildContextNoOverrideWithAllReporter"),
    ])
def test_ReportSettingsOverrides_ValidSettingsScenarios_Success(test_settings_manager_context_for_override_report, mocked_lumberyard_settings, expected_result_map):
    settings_manager.report_settings_overrides(test_settings_manager_context_for_override_report)
    
    assert len(test_settings_manager_context_for_override_report.result_by_attribute) == len(expected_result_map)
    for expected_key in expected_result_map.keys():
        assert expected_key in test_settings_manager_context_for_override_report.result_by_attribute
        assert expected_result_map[expected_key] == test_settings_manager_context_for_override_report.result_by_attribute[expected_key]


@pytest.mark.parametrize(
    "input_messages", [
        pytest.param(["messageA"], id="PrintSettingsOverrideSingleMessage"),
        pytest.param(["messageA", "messageB"], id="PrintSettingsOverrideMultipleMessagesUnique"),
        pytest.param(["messageA", "messageB", "messageA", "messageB"], id="PrintSettingsOverrideMultipleMessagesDuplicate")
    ])
def test_PrintSettingsOverrideMessage_PrintVariousMessages_UniqueMessagesPrinted(input_messages):

    # Arrange
    printed_messages = []

    def _stub_log_pprint(color, msg):
        printed_messages.append(msg)

    old_pprint = Logs.pprint
    Logs.pprint = _stub_log_pprint
    test_context = MockSettingsManagerConfigureContext(run_dir=sys.executable)
    
    # Act
    for input_message in input_messages:
        settings_manager.print_settings_override_message(test_context, input_message)

    Logs.pprint = old_pprint
    
    # Assert
    unique_messages = set(['[SETTINGS] {}'.format(input_message) for input_message in input_messages])
    
    assert len(printed_messages) == len(unique_messages)
    for printed_message in printed_messages:
        assert printed_message in unique_messages


@pytest.mark.parametrize(
    "attribute, default_value, settings_value, expected_printed", [
        pytest.param('attr', 'default', 'default', False, id="ReportSettingsValueUnchanged"),
        pytest.param('attr', 'default', 'override', True, id="ReportSettingsValueChanged"),
    ])
def test_DefaultReportSettingsOverride_VariousValues_Success(attribute, default_value, settings_value, expected_printed):
    # Arrange
    printed_messages = []
    
    def _stub_print_settings_override_message(msg):
        printed_messages.append(msg)
        
    test_context = MockSettingsManagerConfigureContext(run_dir=sys.executable)
    setattr(test_context, 'print_settings_override_message', _stub_print_settings_override_message)
    
    # Act
    settings_manager.default_report_settings_override(test_context, attribute, default_value, settings_value)
    
    # Assert
    printed = len(printed_messages)
    assert expected_printed == printed

@pytest.mark.parametrize(
    "long_form, short_form, cmd_line, expected", [
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build -i True',                                 'True',                 id="UseShortFormValid"),
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build --use-incredibuild=True',                 'True',                 id="UseLongFormValid"),
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build -i=True',                                 type(Errors.WafError),  id="UseShortFormErrorWithValue"),
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build -i=',                                     type(Errors.WafError),  id="UseShortFormErrorWithOutValue"),
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build -i True',                                 'True',                 id="UseShortFormValid"),
        
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build --use-incredibuild=True --foo-arg=False', 'True',                 id="UseLongFormValidWithTrailingArgs"),
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build -i=True --foo-arg=False',                  type(Errors.WafError), id="UseShortFormErrorWithValueWithTrailingArgs"),
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build -i=  --foo-arg=False',                     type(Errors.WafError), id="UseShortFormErrorWithOutValueWithTrailingArgs"),
        
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build --use-incredibuild=',                      '',                    id="UseLongFormValidSetToEmpty"),
        pytest.param('--use-incredibuild', '-i', 'lmbr_waf build --use-incredibuild= --foo-arg=False',      '',                    id="UseLongFormValidSetToEmptyWithTrailingArgs"),
    
    ])
def test_SettingsApplyOptionalOverride_Success(long_form, short_form, cmd_line, expected):
    arguments = cmd_line.split()
    if isinstance(expected, str):
        result = settings_manager.Settings.apply_optional_override(long_form=long_form,
                                                                   short_form=short_form,
                                                                   arguments=arguments)
        assert expected == result
        
    elif isinstance(expected, type(Errors.WafError)):
        with pytest.raises(Errors.WafError):
            settings_manager.Settings.apply_optional_override(long_form=long_form,
                                                              short_form=short_form,
                                                              arguments=arguments)


@pytest.mark.parametrize(
    "name, is_monolithic, is_test, is_server, third_party_config, has_base_config, test_default_settings_map, test_settings_map, expected_name", [
        pytest.param('test_a', False, False, False, 'debug', False, {},{'output_folder_ext_test_a': 'ext_test_a'},   'test_a',                id="ConfigNoMonolithicNoTestNoServerNoBaseConfig"),
        pytest.param('test_b', True, False, False, 'debug', False, {},{'output_folder_ext_test_b': 'ext_test_b'},    'test_b',                id="ConfigMonolithicNoTestNoServerNoBaseConfig"),
        pytest.param('test_c', False, True, False, 'release', False, {}, {'output_folder_ext_test_c': 'ext_test_c'}, 'test_c_test',           id="ConfigNoMonolithicTestNoServerNoBaseConfig"),
        pytest.param('test_d', False, False, True, 'release', False, {}, {'output_folder_ext_test_d': 'ext_test_d'}, 'test_d_dedicated',      id="ConfigNoMonolithicNoTestServerNoBaseConfig"),
        pytest.param('test_e', False, True, True, 'debug', False, {}, {'output_folder_ext_test_e': 'ext_test_e'},    'test_e_test_dedicated', id="ConfigNotMonolithicTestServerNoBaseConfig"),
        pytest.param('test_f', False, False, False, 'release', True, {}, {'output_folder_ext_test_f': 'ext_test_f'}, 'test_f',                id="ConfigNoMonolithicNoTestNoServerBaseConfig")
    ]
)
def test_ConfigurationSettings_ValidPermutations_Success(mocked_lumberyard_settings, name, is_monolithic, is_test, is_server, third_party_config, has_base_config, test_default_settings_map, test_settings_map, expected_name):
    
    base_config_name = 'base_{}'.format(name)
    base_config = settings_manager.ConfigurationSettings(base_config_name, is_monolithic, is_test, third_party_config,None) if has_base_config else None
    test_config = settings_manager.ConfigurationSettings(name, is_monolithic, is_test, third_party_config, base_config)
    expected_folder_ext = 'ext_{}'.format(name)
    
    assert test_config.name == name
    assert test_config.is_monolithic == is_monolithic
    assert test_config.third_party_config == third_party_config

    config_name = test_config.build_config_name(is_test, is_server)
    assert config_name ==  expected_name
    
    output_ext = test_config.get_output_folder_ext()
    assert output_ext == expected_folder_ext
    assert not test_config.does_configuration_match("__foo__")
    assert test_config.does_configuration_match(name)
    if has_base_config:
        assert test_config.does_configuration_match(base_config_name)
        assert not test_config.does_configuration_match(base_config_name, False)


@pytest.mark.parametrize(
    "src_dict, configuration, expected", [
        pytest.param(   {
                            "INCLUDES": [
                                "include_a"
                            ],
                            "DEFINES": [
                                "define_a"
                            ]
                        },
                        "debug",
                        {
                            'debug': {
                                'INCLUDES': [
                                    'include_a'
                                ],
                                'DEFINES': [
                                    'define_a'
                                ]
                            }
                        },
                        id="SimpleDebugListEnvironments"),
        pytest.param(   {
                            "SPECIAL_NAME_A": "include_a",
                            "DEFINES": [
                                "define_a"
                            ]
                        },
                        "profile",
                        {
                            'profile': {
                                "SPECIAL_NAME_A": "include_a",
                                'DEFINES': [
                                    'define_a'
                                ]
                            }
                        },
                        id="SimpleDebugListEnvironments"),
        pytest.param(   {
                            "?CONDITION1?:INCLUDES": [
                                "include_a"
                            ],
                            "@CONDITION2@:DEFINES": [
                                "define_a"
                            ]
                        },
                        "debug",
                        {
                            'debug': {
                                '@CONDITION2@:DEFINES': [
                                    'define_a'
                                ],
                                '?CONDITION1?:INCLUDES': [
                                    'include_a'
                                ]
                            }
                        },
                        id="ConditionalDebugListEnvironments"),
        pytest.param(   {
                            "?CONDITION1?:SPECIAL_NAME_A": "include_a",
                            "DEFINES": [
                                "define_a"
                            ]
                        },
                        "profile",
                        {
                            'profile': {
                                '?CONDITION1?:SPECIAL_NAME_A': 'include_a',
                                'DEFINES': [
                                    'define_a'
                                ]
                            }
                        },
                        id="ConditionalSimpleDebugListEnvironments"),
    ]
)
def test_ProcessEnvDictValues_ValidInputs_Success(src_dict, configuration, expected):
    
    env_result = {}

    settings_manager.process_env_dict_values(src_dict, configuration, env_result)
    
    assert env_result == expected


@pytest.mark.parametrize(
    "src_dict, expected_configurations", [
        pytest.param(   {
                            "env": {
                                "name": "all"
                            }
                        },
                        ['_'],
                        id="SimpleAllEnv"),
        pytest.param(   {
                            "env": {
                                "name": "all"
                            },
                            "env/debug": {
                                "name": "debug"
                            }
                        },
                        ['_', 'debug'],
                        id="AllAndDebugEnv"),
        pytest.param(   {
                            "env": {
                                "name": "all"
                            },
                            "env/debug": {
                                "name": "debug"
                            },
                            "settings": {
                                "name": "dont_include"
                            }
                        },
                        ['_', 'debug'],
                        id="AllAndDebugSkipNonEnv"),
    
    ]
)
def test_ProcessEnvDict_ValidInputs_Success(src_dict, expected_configurations):
    
    def _mock_process_env_dict(env_dict, configuration, processed_env_dict):
        assert configuration in expected_configurations
        if configuration=='_':
            check_key = 'env'
        else:
            check_key = 'env/{}'.format(configuration)

        assert src_dict[check_key] == env_dict
    
    old_process_env_dict = settings_manager.process_env_dict_values
    settings_manager.process_env_dict_values = _mock_process_env_dict
    try:
        result = {}
        settings_manager.process_env_dict(src_dict, result)
    finally:
        settings_manager.process_env_dict_values = old_process_env_dict
    
        
@pytest.mark.parametrize(
    "source_attr_dict, merged_dict, expected_result, expected_warning", [
        pytest.param(   {
                            'NEW_SETTING': 'new'
                        },
                        {
                            'OLD_SETTING': 'old'
                        },
                        {
                            'NEW_SETTING': 'new',
                            'OLD_SETTING': 'old'
                        },
                        False,
                        id="MergeNoOverwrite"),
        pytest.param(   {
                            'NEW_SETTING': 'new'
                        },
                        {
                            'NEW_SETTING': 'new'
                        },
                        {
                            'NEW_SETTING': 'new'
                        },
                        False,
                        id="MergeOverwriteNoChange"),
        pytest.param(   {
                            'NEW_SETTING': 'new'
                        },
                        {
                            'NEW_SETTING': 'old'
                        },
                        {
                            'NEW_SETTING': 'new'
                        },
                        True,
                        id="MergeOverwrite")
    ]
)
def test_MergeAttributesGroup_ValidInputs_Success(source_attr_dict, merged_dict, expected_result, expected_warning):
 
    def _mock_log_warn(msg):
        assert expected_warning

    old_log_warn = Logs.warn
    Logs.warn = _mock_log_warn
    try:
        settings_manager.merge_attributes_group("includes_file", source_attr_dict, merged_dict)
    finally:
        Logs.warn = old_log_warn
    
    assert merged_dict == expected_result


@pytest.mark.parametrize(
    "merge_settings_dict, setting_group_name, settings_value_dicts, expected_result", [
        pytest.param(   {},
                        'group',
                        [],
                        {
                            'group': []
                        },
                        id="EmptyTest"),
        pytest.param(   {
                            'group': []
                        },
                        'group',
                        [],
                        {
                            'group': []
                        },
                        id="EmptyExistingGroupTest"),
        pytest.param(   {
                    
                        },
                        'group',
                        [{
                            "short_form": "-f",
                            "long_form": "--foo",
                            "attribute": "foo",
                            "default_value": "foo",
                            "description": "Use Foo"
                        }],
                        {
                            'group': [
                                {
                                    "short_form": "-f",
                                    "long_form": "--foo",
                                    "attribute": "foo",
                                    "default_value": "foo",
                                    "description": "Use Foo"
                                },
                            ]
                        },
                        id="NewGroupItem"),
        pytest.param(   {
                            'group': [
                                {
                                    "short_form": "-n",
                                    "long_form": "--not-foo",
                                    "attribute": "not_foo",
                                    "default_value": "not_foo",
                                    "description": "Use Not Foo"
                                },
                            ]
                        },
                        'group',
                        [{
                            "short_form": "-f",
                            "long_form": "--foo",
                            "attribute": "foo",
                            "default_value": "foo",
                            "description": "Use Foo"
                        }],
                        {
                            'group': [
                                {
                                    "short_form": "-n",
                                    "long_form": "--not-foo",
                                    "attribute": "not_foo",
                                    "default_value": "not_foo",
                                    "description": "Use Not Foo"
                                },
                                {
                                    "short_form": "-f",
                                    "long_form": "--foo",
                                    "attribute": "foo",
                                    "default_value": "foo",
                                    "description": "Use Foo"
                                },
                            ]
                        },
                        id="NewGroupItemExistingGroup"),
        pytest.param(   {
                            'group': [
                                {
                                    "short_form": "-f",
                                    "long_form": "--foo",
                                    "attribute": "foo",
                                    "default_value": "old_foo",
                                    "description": "Use Old Foo"
                                },
                            ]
                        },
                        'group',
                        [{
                            "short_form": "-f",
                            "long_form": "--foo",
                            "attribute": "foo",
                            "default_value": "new_foo",
                            "description": "Use New Foo"
                        }],
                        {
                            'group': [
                                {
                                    "short_form": "-f",
                                    "long_form": "--foo",
                                    "attribute": "foo",
                                    "default_value": "new_foo",
                                    "description": "Use New Foo"
                                },
                            ]
                        },
                        id="ReplaceExistingGroupItem")
    ])
def test_MergeSettingsGroup_ValidInputs_Success(merge_settings_dict, setting_group_name, settings_value_dicts, expected_result):
    
    settings_manager.merge_settings_group('includes_file', merge_settings_dict, setting_group_name, settings_value_dicts)
    
    assert merge_settings_dict == expected_result


@pytest.mark.parametrize(
    "settings_include_file, include_to_settings_dict, expected_env_dict, expected_settings_dict, expected_attributes_dict", [
        pytest.param(   'testpath/test_settings.json',
                        {
                            'testpath/test_settings.json': {}
                        },
                        {},
                        {},
                        {},
                        id="EmptySettings"),
        
        pytest.param(   'testpath/test_settings.json',
                        {
                            'testpath/test_settings.json': {
                                'env': {
                                    'MYKEY': 'MYVALUE'
                                }
                            }
                        },
                        {
                             'env': {
                                'MYKEY': 'MYVALUE'
                            }
                        },
                        {},
                        {},
                        id="ProcessEnv"),
        
        pytest.param(   'testpath/test_settings.json',
                        {
                            'testpath/test_settings.json': {
                                "settings": {
                                    "MYSETTING": "SETTINGVALUE"
                                },
                                "env": {
                                    "MYKEY": "MYVALUE"
                                },
                                "attributes": {
                                    "MYATTRIBUTE": "ATTRIBUTEVALUE"
                                }
                            }
                        },
                        {
                            'env': {
                                'MYKEY': 'MYVALUE'
                            }
                        },
                        {
                            'MYSETTING': 'SETTINGVALUE'
                        },
                        {
                            'MYATTRIBUTE': 'ATTRIBUTEVALUE'
                        },
                        id="ProcessEnvAttributesSettings"),
        pytest.param(   'testpath/test_settings.json',
                        {
                            'testpath/include_test_settings.json': {
                                "settings": {
                                    "MYSETTING": "SETTINGVALUE"
                                },
                                "env": {
                                    "MYKEY": "MYVALUE"
                                },
                                "attributes": {
                                    "MYATTRIBUTE": "ATTRIBUTEVALUE"
                                }
                            },
                            'testpath/test_settings.json': {
                                "includes": [
                                    'include_test_settings.json'
                                ]
                            }
                        },
                        {
                            'env': {
                                'MYKEY': 'MYVALUE'
                            }
                        },
                        {
                            'MYSETTING': 'SETTINGVALUE'
                        },
                        {
                            'MYATTRIBUTE': 'ATTRIBUTEVALUE'
                        },
                        id="ProcessEnvAttributesSettingsFromInclude"),
    ]
)
def test_ProcessSettingsIncludeFile_ValidInputs_Success(settings_include_file,
                                                        include_to_settings_dict,
                                                        expected_env_dict,
                                                        expected_settings_dict,
                                                        expected_attributes_dict):
    
    def _mock_read_common_config(input_file):
        match_input_form = input_file.replace('\\', '/')
        assert match_input_form in include_to_settings_dict
        return include_to_settings_dict[match_input_form]
    
    def _mock_process_env_dict(settings_include_dict, processed_env_dict):
        for env_key, env_value in settings_include_dict.items():
            if env_key.startswith('env'):
                processed_env_dict[env_key] = env_value
    
    def _mock_merge_settings_group(settings_include_file,
                                   merge_settings_dict,
                                   setting_group_name,
                                   settings_value_dicts):
        merge_settings_dict[setting_group_name] = settings_value_dicts

    def _mock_merge_attributes_group(settings_include_file,
                                     source_attributes_dict,
                                     merge_attributes_dict):
        if source_attributes_dict:
            for key, value in source_attributes_dict.items():
                merge_attributes_dict[key] = value

    
    processed_env_dict = {}
    processed_settings_dict = {}
    processed_attributes_dict = {}
    
    old_read_common_config = settings_manager.read_common_config
    old_process_env_dict = settings_manager.process_env_dict
    old_merge_settings_group = settings_manager.merge_settings_group
    old_merge_attributes_group = settings_manager.merge_attributes_group

    settings_manager.read_common_config = _mock_read_common_config
    settings_manager.process_env_dict = _mock_process_env_dict
    settings_manager.merge_settings_group = _mock_merge_settings_group
    settings_manager.merge_attributes_group = _mock_merge_attributes_group
    
    try:
        settings_manager.process_settings_include_file(settings_include_file,
                                                       processed_env_dict,
                                                       processed_settings_dict,
                                                       processed_attributes_dict)
    finally:
        settings_manager.read_common_config = old_read_common_config
        settings_manager.process_env_dict = old_process_env_dict
        settings_manager.merge_settings_group = old_merge_settings_group
        settings_manager.merge_attributes_group = old_merge_attributes_group

    assert expected_env_dict == processed_env_dict
    assert expected_settings_dict == processed_settings_dict
    assert expected_attributes_dict == processed_attributes_dict
    
    
@pytest.mark.parametrize(
    "platform_settings_file, platform_file_to_settings_dict,expected_platform,expected_display_name,expected_hosts,"
    "expected_aliases,expected_has_server,expected_has_test,expected_is_monolithic,expected_enabled,"
    "expected_additional_modules,expected_needs_java, expected_env_dict, expected_settings_dict, "
    "expected_attributes_dict", [
        pytest.param(   'testpath/test_settings.json',
                        {
                            'testpath/test_settings.json': {
                                'platform': 'test_platform',
                                'hosts': 'win32'
                            }
                        },
                        'test_platform',        # expected_platform,
                        'test_platform',        # expected_display_name, (default)
                        'win32',                # expected_hosts,
                        set(),                  # expected_aliases, (default)
                        False,                  # expected_has_server, (default)
                        False,                  # expected_has_test, (default)
                        False,                  # expected_is_monolithic, (default)
                        True,                   # expected_enabled, (default)
                        [],                     # expected_additional_modules, (default)
                        False,                  # expected_needs_java (default)
                        {},                     # expected_env_dict (default)
                        {},                     # expected_settings_dict (default)
                        {},                     # expected_attributes_dict (default)
                        id="BasicDefaultSettings"
                     ),
        pytest.param(   'testpath/test_settings.json',
                        {
                            'testpath/test_settings.json': {
                                'platform': 'test_platform',
                                'display_name': 'display_test',
                                'hosts': 'win32',
                                'aliases': 'msvc',
                                'has_server': True,
                                'has_tests': True,
                                'is_monolithic': True,
                                'enabled': False,
                                'needs_java': True,
                                'modules': [
                                    'module_test'
                                ],
                                "settings": {
                                    "MYSETTING": "SETTINGVALUE"
                                },
                                "env": {
                                    "MYKEY": "MYVALUE"
                                },
                                "attributes": {
                                    "MYATTRIBUTE": "ATTRIBUTEVALUE"
                                }
                            }
                        },
                        'test_platform',                    # expected_platform
                        'display_test',                     # expected_display_name
                        'win32',                            # expected_hosts
                        {'msvc'},                           # expected_aliases
                        True,                               # expected_has_server
                        True,                               # expected_has_test
                        True,                               # expected_is_monolithic
                        False,                              # expected_enabled
                        ['module_test'],                    # expected_additional_modules
                        True,                               # expected_needs_java (default)
                        {'env': {'MYKEY': 'MYVALUE'}},      # expected_env_dict (default)
                        {'MYSETTING': 'SETTINGVALUE'},      # expected_settings_dict (default)
                        {'MYATTRIBUTE': 'ATTRIBUTEVALUE'},  # expected_attributes_dict (default)
                        id="BasicTestingSettings"),
        pytest.param(   'testpath/test_settings.json',
                        {
                            'testpath/test_settings.json': {
                                'platform': 'test_platform',
                                'display_name': 'display_test',
                                'hosts': 'win32',
                                'aliases': 'msvc',
                                'has_server': True,
                                'has_tests': True,
                                'is_monolithic': True,
                                'enabled': False,
                                'needs_java': True,
                                'modules': ['module_test'],
                                'includes': [
                                    'test_includes.json'
                                ]
                            },
                            'testpath/test_includes.json': {
                                 "settings": {
                                     "MYSETTING": "SETTINGVALUE"
                                 },
                                 "env": {
                                     "MYKEY": "MYVALUE"
                                 },
                                 "attributes": {
                                     "MYATTRIBUTE": "ATTRIBUTEVALUE"
                                 }
                            }
                        },
                        'test_platform',                    # expected_platform
                        'display_test',                     # expected_display_name
                        'win32',                            # expected_hosts
                        {'msvc'},                           # expected_aliases
                        True,                               # expected_has_server
                        True,                               # expected_has_test
                        True,                               # expected_is_monolithic
                        False,                              # expected_enabled
                        ['module_test'],                    # expected_additional_modules
                        True,                               # expected_needs_java (default)
                        {'env': {'MYKEY': 'MYVALUE'}},      # expected_env_dict (default)
                        {'MYSETTING': 'SETTINGVALUE'},      # expected_settings_dict (default)
                        {'MYATTRIBUTE': 'ATTRIBUTEVALUE'},  # expected_attributes_dict (default)
                        id="BasicTestingSettingsFromIncludes")
    ]
)
def test_PlatformSettings_ValidInputs_Success(platform_settings_file, platform_file_to_settings_dict, expected_platform, expected_display_name,
                                              expected_hosts, expected_aliases, expected_has_server, expected_has_test, expected_is_monolithic,
                                              expected_enabled, expected_additional_modules, expected_needs_java, expected_env_dict,
                                              expected_settings_dict, expected_attributes_dict):
    
    def _mock_parse_json_file(input_file, ignore_comments):
        match_input_form = input_file.replace('\\', '/')
        assert match_input_form in platform_file_to_settings_dict
        return platform_file_to_settings_dict[match_input_form]

    def _mock_process_env_dict(settings_include_dict, processed_env_dict):
        for env_key, env_value in settings_include_dict.items():
            if env_key.startswith('env'):
                processed_env_dict[env_key] = env_value

    def _mock_merge_settings_group(settings_include_file,
                                   merge_settings_dict,
                                   setting_group_name,
                                   settings_value_dicts):
        merge_settings_dict[setting_group_name] = settings_value_dicts

    def _mock_merge_attributes_group(settings_include_file,
                                     source_attributes_dict,
                                     merge_attributes_dict):
        if source_attributes_dict:
            for key, value in source_attributes_dict.items():
                merge_attributes_dict[key] = value

    old_parse_json_file = utils.parse_json_file
    old_process_env_dict = settings_manager.process_env_dict
    old_merge_settings_group = settings_manager.merge_settings_group
    old_merge_attributes_group = settings_manager.merge_attributes_group
    
    utils.parse_json_file = _mock_parse_json_file
    settings_manager.process_env_dict = _mock_process_env_dict
    settings_manager.merge_settings_group = _mock_merge_settings_group
    settings_manager.merge_attributes_group = _mock_merge_attributes_group
    
    try:
        platform_settings = settings_manager.PlatformSettings(platform_settings_file)
    finally:
        utils.parse_json_file = old_parse_json_file
        settings_manager.process_env_dict = old_process_env_dict
        settings_manager.merge_settings_group = old_merge_settings_group
        settings_manager.merge_attributes_group = old_merge_attributes_group
        
    assert platform_settings.hosts == expected_hosts
    assert platform_settings.aliases == expected_aliases
    assert platform_settings.has_server == expected_has_server
    assert platform_settings.has_test == expected_has_test
    assert platform_settings.enabled == expected_enabled
    assert platform_settings.additional_modules == expected_additional_modules
    assert platform_settings.needs_java == expected_needs_java

    assert platform_settings.env_dict == expected_env_dict
    assert platform_settings.settings == expected_settings_dict
    assert platform_settings.attributes == expected_attributes_dict


@pytest.fixture
def mock_settings(tmpdir, default_settings, user_settings_options, build_configurations, test_platform_name, test_platform_content):
    
    def write_json_sample(input_dict, target_path):
        json_file_content = json.dumps(input_dict,
                                       sort_keys=True,
                                       separators=(',', ': '),
                                       indent=4)
        target_path.write(json_file_content)

    tmpdir_path = str(tmpdir.realpath())

    tmpdir.ensure('_WAF_/settings/platforms', dir=True)

    write_json_sample(default_settings, tmpdir.join('_WAF_/default_settings.json'))
    
    tmpdir.join('_WAF_/user_settings.options').write(user_settings_options)

    write_json_sample(build_configurations, tmpdir.join('_WAF_/settings/build_configurations.json'))
    
    write_json_sample(test_platform_content, tmpdir.join('_WAF_/settings/platforms/platform.{}.json'.format(test_platform_name)))
    
    
    def _mock_get_cwd():
        return tmpdir_path
    
    def _mock_unversioned_sys_platform():
        return 'win32'
    
    old_get_cwd = os.getcwd
    old_unversioned_sys_platform = Utils.unversioned_sys_platform
    
    os.getcwd = _mock_get_cwd
    Utils.unversioned_sys_platform = _mock_unversioned_sys_platform
    try:
        settings = settings_manager.Settings()
    finally:
        os.getcwd = old_get_cwd
        Utils.unversioned_sys_platform = old_unversioned_sys_platform
        
    yield settings

    tmpdir.remove(ignore_errors=True)
    
    



MIN_DEFAULT_SETTINGS = {
    "General": [
        {
            "long_form": "--has-test-configs",
            "attribute": "has_test_configs",
            "default_value": "False",
            "description": "Has Test Configs"
        },
        {
            "long_form": "--has-server-configs",
            "attribute": "has_server_configs",
            "default_value": "False",
            "description": "Has Server Configs"
        }
    ]
}


def join_with_default(input):
    
    combined_items = input.items() + MIN_DEFAULT_SETTINGS.items()
    
    result = {}
    
    for section_name, section_list in combined_items:
        if section_name not in result:
            result[section_name] = copy.deepcopy(section_list)
        else:
            for subsection in section_list:
                result[section_name].append(copy.deepcopy(subsection))
    return result


BASIC_TEST_CONFIGURATIONS = {
    "configurations": {
        "debug": {
            "is_monolithic": False,
            "has_test": True,
            "third_party_config": "debug",
            "default_output_ext": "Debug"
        },
        "profile": {
            "is_monolithic": False,
            "has_test": True,
            "third_party_config": "release",
            "default_output_ext": ""
        },
        "performance": {
            "is_monolithic": True,
            "has_test": False,
            "third_party_config": "release",
            "default_output_ext": "Performance"
        },
        "release": {
            "is_monolithic": True,
            "has_test": False,
            "third_party_config": "release",
            "default_output_ext": "Release"
        }
    }
}

    
@pytest.mark.parametrize(
    "default_settings, user_settings_options, build_configurations, test_platform_name, test_platform_content, expected_settings", [
        pytest.param(   join_with_default({
                            "General": [
                                {
                                    "long_form": "--foo",
                                    "attribute": "foo",
                                    "default_value": "Foo",
                                    "description": "Use New Foo"
                                }
                            ]
                        }),
                        '[General]\n'
                        'foo = Bar\n'
                        ,
                        BASIC_TEST_CONFIGURATIONS,
                        'test_platform',
                        {
                            'platform': 'test_platform',
                            'hosts': 'win32',
                            "attributes": {
                                "default_folder_name": "BinTest"
                            }
                        },
                        {
                            'foo': 'Bar',
                            'out_folder_test_platform': 'BinTest'
                        }
                    )
    ]
)
def test_Settings_BasicTest_Success(tmpdir, mock_settings, default_settings, user_settings_options, build_configurations, test_platform_name, test_platform_content, expected_settings):
    
    for expected_key, expected_value in expected_settings.items():
        assert expected_key in mock_settings.settings_map
        assert mock_settings.settings_map[expected_key] == expected_value
    assert test_platform_name in mock_settings.platform_settings_map

