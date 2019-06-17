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
import lumberyard_modules
import unittest
import pytest

import utils


class FakeContext(object):
    pass


class FakeIncludeSettings(object):
    pass


class FakePlatformSettings(object):
    def __init__(self, platform_name, aliases=set()):
        self.platform = platform_name
        self.aliases = aliases

        
class FakeConfigurationSettings(object):
    
    def __init__(self, settings_name, base_config=None):
        self.base_config = base_config
        self.name = settings_name

        
class FakeConfiguration(object):
    def __init__(self, settings, is_test=False, is_server=False):
        self.settings = settings
        self.is_test = is_test
        self.is_server = is_server


@pytest.fixture()
def mock_parse_json(mock_json_map):
    
    if not mock_json_map:
        mock_json_map = {'path': {}}
    
    def _mock_parse_json(path, _):
        return mock_json_map[path]
    
    old_parse_json_file = utils.parse_json_file
    utils.parse_json_file = _mock_parse_json
    
    yield
    
    utils.parse_json_file = old_parse_json_file
    

@pytest.fixture()
def fake_context():
    return FakeContext()


def test_SanitizeKWInput_SimpleKwDictionary_Success():
    kw = dict(
        libpath='mylib'
    )
    lumberyard_modules.sanitize_kw_input(kw)
    assert isinstance(kw['libpath'], list)
    assert kw['libpath'][0] == 'mylib'


def test_SanitizeKWInput_SimpleKwDictionaryInAdditionalSettings_Success():
    kw = dict(
        libpath='mylib',
        additional_settings=dict(stlibpath='mystlib')
    )
    lumberyard_modules.sanitize_kw_input(kw)
    assert isinstance(kw['libpath'], list)
    assert kw['libpath'][0] == 'mylib'
    assert isinstance(kw['additional_settings'], list)
    assert isinstance(kw['additional_settings'][0], dict)
    assert isinstance(kw['additional_settings'][0]['stlibpath'], list)
    assert kw['additional_settings'][0]['stlibpath'][0] == 'mystlib'


@pytest.mark.parametrize(
                     "target,       kw_key,          source_section,             additional_aliases, merge_dict,                expected", [
        pytest.param('test_target', 'fake_key',      {},                         {},                 {},                        {},                         id='MissingKeyInSourceNoChange'),
        pytest.param('test_target', 'fake_key',      {'fake_key': 'fake_value'}, {},                 {},                        {'fake_key': 'fake_value'}, id='MissingKeyInTargetKeyAdded'),
        pytest.param('test_target', 'copyright_org', {'copyright_org': False},   {},                 {'copyright_org': 'AMZN'}, type(Errors.WafError),      id='InvalidStringKwInSourceError'),
        pytest.param('test_target', 'copyright_org', {'copyright_org': 'AMZN'},  {},                 {'copyright_org': False},  type(Errors.WafError),      id='InvalidStringKwInTargetError'),
        pytest.param('test_target', 'copyright_org', {'copyright_org': 'AMZN'},  {},                 {'copyright_org': 'A2Z'},  {'copyright_org': 'AMZN'},  id='MergeStringReplaceSuccess'),
        pytest.param('test_target', 'client_only',   {'client_only': 'False'},   {},                 {'client_only': True},     type(Errors.WafError),      id='InvalidBoolKwInSourceError'),
        pytest.param('test_target', 'client_only',   {'client_only': False},     {},                 {'client_only': 'True'},   type(Errors.WafError),      id='InvalidBoolKwInTargetError'),
        pytest.param('test_target', 'client_only',   {'client_only': False},     {},                 {'client_only': True},     {'client_only': False},     id='MergeBoolReplaceKwSuccess'),
    ])
def test_ProjectSettingsFileMergeKwKey_ValidInputs(mock_parse_json, target, kw_key, source_section, additional_aliases, merge_dict, expected):

    fake_context = FakeContext()
    test_settings = lumberyard_modules.ProjectSettingsFile(fake_context, 'path', additional_aliases)

    if isinstance(expected,dict):
        test_settings.merge_kw_key(target=target,
                                   kw_key=kw_key,
                                   source_section=source_section,
                                   merge_kw=merge_dict)
        assert merge_dict == expected
    elif isinstance(expected, type(Errors.WafError)):
        with pytest.raises(Errors.WafError):
            test_settings.merge_kw_key(target=target,
                                       kw_key=kw_key,
                                       source_section=source_section,
                                       merge_kw=merge_dict)


@pytest.mark.parametrize(
    "test_dict, fake_include_settings, mock_json_map, additional_aliases, expected", [
        pytest.param({}, None, None, {}, {}, id='BasicNoAdditionalAliasNoAdditionalIncludes'),
        pytest.param({}, 'include_test',
                          {
                            'path': {
                                'includes': ['include_test']
                            },'include_test': {}
                          }, {}, {'includes': ['include_test']}, id='BasicNoAdditionalAliasSingleAdditionalIncludes')
    ])
def test_ProjectSettingsFileMergeKwKey_ValidInputs(mock_parse_json, fake_context, test_dict, fake_include_settings, mock_json_map, additional_aliases, expected):
    
    if fake_include_settings:
        def _mock_get_project_settings_file(include_settings_file, additional_aliases):
            assert fake_include_settings == include_settings_file
            fake_settings = FakeIncludeSettings()
            return fake_settings
        fake_context.get_project_settings_file = _mock_get_project_settings_file
    
    test = lumberyard_modules.ProjectSettingsFile(fake_context,
                                                  'path',
                                                  additional_aliases)
    
    assert test.dict == expected


@pytest.mark.parametrize(
    "mock_json_map, additional_aliases, section_key, expected", [
        pytest.param(None, {}, 'no_section', {}, id='SimpleNoChange'),
        pytest.param({
                        'path': {
                            "test_section": {
                                "key1": "value1"
                            }
                        }
                     }, {}, 'test_section', {'key1': 'value1'}, id='SimpleChanges')
    ])
def test_ProjectSettingsFileMergeKwSection_ValidInputs_Success(mock_parse_json, fake_context, mock_json_map, additional_aliases, section_key, expected):

    test_settings = lumberyard_modules.ProjectSettingsFile(fake_context, 'path', additional_aliases)

    merge_dict = {}

    test_settings.merge_kw_section(section_key=section_key,
                                   target='test_target',
                                   merge_kw=merge_dict)
    
    assert expected == merge_dict
    
    
    
    
    


class ProjectSettingsTest(unittest.TestCase):
    
    def setUp(self):
        self.old_parse_json = utils.parse_json_file
        utils.parse_json_file = self.mockParseJson
        self.mock_json_map = {}
    
    def tearDown(self):
        utils.parse_json_file = self.old_parse_json
    
    def mockParseJson(self, path, _):
        return self.mock_json_map[path]

    def createSimpleSettings(self, fake_context = FakeContext(), test_dict={}, additional_aliases={}):
        
        self.mock_json_map = {'path': test_dict}

        test_settings = lumberyard_modules.ProjectSettingsFile(fake_context, 'path', additional_aliases)
        
        return test_settings

    def test_ProjectSettingsFileMergeKwDict_RecursiveMergeAdditionalSettingsNoPlatformNoConfiguration_Success(self):
        """
        Test scenario:
        
        Setup a project settings that contains other project settings, so that it can recursively call merge_kw_dict
        recursively

        """

        include_settings_file = 'include_test'
        test_settings_single_include = {'includes': [include_settings_file]}
        test_empty_settings = {}
        test_merge_kw_key = 'passed'
        test_merge_kw_value = True
    
        self.mock_json_map = {'path': test_settings_single_include,
                              include_settings_file: test_empty_settings}
        
        # Prepare a mock include settings object
        test_include_settings = self.createSimpleSettings()
        def _mock_merge_kw_dict(target, merge_kw, platform, configuration):
            merge_kw[test_merge_kw_key] = test_merge_kw_value
            pass
        test_include_settings.merge_kw_dict = _mock_merge_kw_dict

        # Prepare a mock context
        fake_context = FakeContext()
        def _mock_get_project_settings_file(_a, _b):
            return test_include_settings
        fake_context.get_project_settings_file = _mock_get_project_settings_file
        
        test_settings = self.createSimpleSettings(fake_context=fake_context,
                                                  test_dict=test_settings_single_include)

        test_merge_kw = {}
        test_settings.merge_kw_dict(target='test_target',
                                    merge_kw=test_merge_kw,
                                    platform=None,
                                    configuration=None)
        
        self.assertIn(test_merge_kw_key, test_merge_kw)
        self.assertEqual(test_merge_kw[test_merge_kw_key], test_merge_kw_value)

    def test_ProjectSettingsFileMergeKwDict_MergePlatformSection_Success(self):
        """
        Test scenario:
        
        Test the merge_kw_dict when only platform is set and not any configurations

        """

        test_platform = 'test_platform'
        test_alias = 'alias_1'
        
        fake_context = FakeContext()

        fake_platform_settings = FakePlatformSettings(platform_name='test_platform',
                                                      aliases={test_alias})
        def _mock_get_platform_settings(platform):
            self.assertEqual(platform, test_platform)
            return fake_platform_settings

        fake_context.get_platform_settings = _mock_get_platform_settings

        test_dict = {}
        test_settings = self.createSimpleSettings(fake_context=fake_context,
                                                  test_dict=test_dict)
        sections_merged = set()
        def _mock_merge_kw_section(section, target, merge_kw):
            sections_merged.add(section)
            pass

        test_settings.merge_kw_section = _mock_merge_kw_section
        
        test_merge_kw = {}
        test_settings.merge_kw_dict(target='test_target',
                                    merge_kw=test_merge_kw,
                                    platform=test_platform,
                                    configuration=None)
        
        # Validate all the sections passed to the merge_kw_dict
        self.assertIn('{}/*'.format(test_platform), sections_merged)
        self.assertIn('{}/*'.format(test_alias), sections_merged)
        self.assertEqual(len(sections_merged), 2)

    def test_ProjectSettingsFileMergeKwDict_MergePlatformConfigurationNoDerivedNoTestNoDedicatedSection_Success(self):
        """
        Test scenario:

        Test the merge_kw_dict when the platform + configuration is set, and the configuration is not a test nor
        server configuration
        """
    
        test_platform_name = 'test_platform'
        test_configuration_name = 'test_configuration'

        test_configuration = FakeConfiguration(settings=FakeConfigurationSettings(settings_name=test_configuration_name))

        fake_context = FakeContext()
    
        fake_platform_settings = FakePlatformSettings(platform_name='test_platform')
    
        def _mock_get_platform_settings(platform):
            self.assertEqual(platform, test_platform_name)
            return fake_platform_settings
    
        fake_context.get_platform_settings = _mock_get_platform_settings
    
        test_dict = {}
        test_settings = self.createSimpleSettings(fake_context=fake_context,
                                                  test_dict=test_dict)
        sections_merged = set()
    
        def _mock_merge_kw_section(section, target, merge_kw):
            sections_merged.add(section)
            pass
    
        test_settings.merge_kw_section = _mock_merge_kw_section
    
        test_merge_kw = {}
        test_settings.merge_kw_dict(target='test_target',
                                    merge_kw=test_merge_kw,
                                    platform=test_platform_name,
                                    configuration=test_configuration)
    
        # Validate all the sections passed to the merge_kw_dict
        self.assertIn('{}/*'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertEqual(len(sections_merged), 2)

    def test_ProjectSettingsFileMergeKwDict_MergePlatformConfigurationDerivedNoTestNoDedicatedSection_Success(self):
        """
        Test scenario:

        Test the merge_kw_dict when the platform + configuration is set, and the configuration is not a test nor
        server configuration, but is derived from another configuration
        """
        test_platform_name = 'test_platform'
        test_configuration_name = 'test_configuration'
        base_test_configuration_name = 'base_configuration'
    
        test_configuration = FakeConfiguration(
            settings=FakeConfigurationSettings(settings_name=test_configuration_name,
                                               base_config=FakeConfiguration(FakeConfigurationSettings(settings_name=base_test_configuration_name))))
    
        fake_context = FakeContext()
    
        fake_platform_settings = FakePlatformSettings(platform_name='test_platform')
    
        def _mock_get_platform_settings(platform):
            self.assertEqual(platform, test_platform_name)
            return fake_platform_settings
    
        fake_context.get_platform_settings = _mock_get_platform_settings
    
        test_dict = {}
        test_settings = self.createSimpleSettings(fake_context=fake_context,
                                                  test_dict=test_dict)
        sections_merged = set()
    
        def _mock_merge_kw_section(section, target, merge_kw):
            sections_merged.add(section)
            pass
    
        test_settings.merge_kw_section = _mock_merge_kw_section
    
        test_merge_kw = {}
        test_settings.merge_kw_dict(target='test_target',
                                    merge_kw=test_merge_kw,
                                    platform=test_platform_name,
                                    configuration=test_configuration)
    
        # Validate all the sections passed to the merge_kw_dict
        self.assertIn('{}/*'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertIn('{}/{}'.format(test_platform_name, base_test_configuration_name), sections_merged)
        self.assertEqual(len(sections_merged), 3)

    def test_ProjectSettingsFileMergeKwDict_MergePlatformConfigurationNoDerivedTestDedicatedSection_Success(self):
        """
        Test scenario:

        Test the merge_kw_dict when the platform + configuration is set, and the configuration is a test and a
        server configuration
        """
        test_platform_name = 'test_platform'
        test_configuration_name = 'test_configuration'
    
        test_configuration = FakeConfiguration(settings=FakeConfigurationSettings(settings_name=test_configuration_name),
                                               is_test=True,
                                               is_server=True)
    
        fake_context = FakeContext()
    
        fake_platform_settings = FakePlatformSettings(platform_name='test_platform')
    
        def _mock_get_platform_settings(platform):
            self.assertEqual(platform, test_platform_name)
            return fake_platform_settings
    
        fake_context.get_platform_settings = _mock_get_platform_settings
    
        test_dict = {}
        test_settings = self.createSimpleSettings(fake_context=fake_context,
                                                  test_dict=test_dict)
        sections_merged = set()
    
        def _mock_merge_kw_section(section, target, merge_kw):
            sections_merged.add(section)
            pass
    
        test_settings.merge_kw_section = _mock_merge_kw_section
    
        test_merge_kw = {}
        test_settings.merge_kw_dict(target='test_target',
                                    merge_kw=test_merge_kw,
                                    platform=test_platform_name,
                                    configuration=test_configuration)
    
        # Validate all the sections passed to the merge_kw_dict
        self.assertIn('{}/{}'.format(test_platform_name, test_configuration_name), sections_merged)

        self.assertIn('*/*/dedicated,test', sections_merged)
        self.assertIn('{}/*/dedicated,test'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/dedicated,test'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertIn('*/*/test,dedicated', sections_merged)
        self.assertIn('{}/*/test,dedicated'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/test,dedicated'.format(test_platform_name, test_configuration_name), sections_merged)

        self.assertEqual(len(sections_merged), 8)

    def test_ProjectSettingsFileMergeKwDict_MergePlatformConfigurationNoDerivedTestNoDedicatedSection_Success(self):
        """
        Test scenario:

        Test the merge_kw_dict when the platform + configuration is set, and the configuration is a test but not a
        server configuration
        """
        test_platform_name = 'test_platform'
        test_configuration_name = 'test_configuration'
    
        test_configuration = FakeConfiguration(
            settings=FakeConfigurationSettings(settings_name=test_configuration_name),
            is_test=True,
            is_server=False)
    
        fake_context = FakeContext()
    
        fake_platform_settings = FakePlatformSettings(platform_name='test_platform')
    
        def _mock_get_platform_settings(platform):
            self.assertEqual(platform, test_platform_name)
            return fake_platform_settings
    
        fake_context.get_platform_settings = _mock_get_platform_settings
    
        test_dict = {}
        test_settings = self.createSimpleSettings(fake_context=fake_context,
                                                  test_dict=test_dict)
        sections_merged = set()
    
        def _mock_merge_kw_section(section, target, merge_kw):
            sections_merged.add(section)
            pass
    
        test_settings.merge_kw_section = _mock_merge_kw_section
    
        test_merge_kw = {}
        test_settings.merge_kw_dict(target='test_target',
                                    merge_kw=test_merge_kw,
                                    platform=test_platform_name,
                                    configuration=test_configuration)
    
        # Validate all the sections passed to the merge_kw_dict
        self.assertIn('{}/*'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}'.format(test_platform_name, test_configuration_name), sections_merged)
    
        self.assertIn('*/*/test', sections_merged)
        self.assertIn('{}/*/test'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/test'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertIn('*/*/dedicated,test', sections_merged)
        self.assertIn('{}/*/dedicated,test'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/dedicated,test'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertIn('*/*/test,dedicated', sections_merged)
        self.assertIn('{}/*/test,dedicated'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/test,dedicated'.format(test_platform_name, test_configuration_name), sections_merged)

        self.assertEqual(len(sections_merged), 11)

    def test_ProjectSettingsFileMergeKwDict_MergePlatformConfigurationNoDerivedNoTestDedicatedSection_Success(self):
        """
        Test scenario:
        
        Test the merge_kw_dict when the platform + configuration is set, and the configuration is a server but not a
        test configuration
        """
        test_platform_name = 'test_platform'
        test_configuration_name = 'test_configuration'
    
        test_configuration = FakeConfiguration(
            settings=FakeConfigurationSettings(settings_name=test_configuration_name),
            is_test=False,
            is_server=True)
    
        fake_context = FakeContext()
    
        fake_platform_settings = FakePlatformSettings(platform_name='test_platform')
    
        def _mock_get_platform_settings(platform):
            self.assertEqual(platform, test_platform_name)
            return fake_platform_settings
    
        fake_context.get_platform_settings = _mock_get_platform_settings
    
        test_dict = {}
        test_settings = self.createSimpleSettings(fake_context=fake_context,
                                                  test_dict=test_dict)
        sections_merged = set()
    
        def _mock_merge_kw_section(section, target, merge_kw):
            sections_merged.add(section)
            pass
    
        test_settings.merge_kw_section = _mock_merge_kw_section
    
        test_merge_kw = {}
        test_settings.merge_kw_dict(target='test_target',
                                    merge_kw=test_merge_kw,
                                    platform=test_platform_name,
                                    configuration=test_configuration)
    
        # Validate all the sections passed to the merge_kw_dict
        self.assertIn('{}/*'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertIn('*/*/dedicated', sections_merged)
        self.assertIn('{}/*/dedicated'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/dedicated'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertIn('*/*/dedicated,test', sections_merged)
        self.assertIn('{}/*/dedicated,test'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/dedicated,test'.format(test_platform_name, test_configuration_name), sections_merged)
        self.assertIn('*/*/test,dedicated', sections_merged)
        self.assertIn('{}/*/test,dedicated'.format(test_platform_name), sections_merged)
        self.assertIn('{}/{}/test,dedicated'.format(test_platform_name, test_configuration_name), sections_merged)

        self.assertEqual(len(sections_merged), 11)

