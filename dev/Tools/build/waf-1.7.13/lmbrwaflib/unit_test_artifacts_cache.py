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
import pytest
from unittest import mock


# waflib imports
from waflib.ConfigSet import ConfigSet

# lmbrwaflib imports
from lmbrwaflib import artifacts_cache


@pytest.fixture()
def mocked_bld_a():
    bld = mock.Mock()
    tp = mock.Mock(content={'3rdPartyRoot': r'D:\workspace_a\3rdParty'})
    bld.configure_mock(engine_path=r'D:\workspace_a\dev', tp=tp)
    del bld.engine_path_replaced
    del bld.tp_root_replaced
    del bld.cache_env
    return bld


@pytest.fixture()
def mocked_bld_b():
    bld = mock.Mock()
    tp = mock.Mock(content={'3rdPartyRoot': r'D:\workspace_b\3rdParty'})
    bld.configure_mock(engine_path=r'D:\workspace_b\dev', tp=tp)
    del bld.engine_path_replaced
    del bld.tp_root_replaced
    del bld.cache_env
    return bld


@pytest.fixture()
def mocked_env_a(table_a):
    env = ConfigSet()
    env.table = table_a
    return env


@pytest.fixture()
def mocked_env_b(table_b):
    env = ConfigSet()
    env.table = table_b
    return env


@pytest.mark.parametrize(
    "string_a, string_b", [
        (r'D:\workspace_a\dev\engine_root.txt D:\workspace_a\dev\engine.json D:\workspace_a\3rdParty\3rdParty.txt',
         r'D:\workspace_b\dev\engine_root.txt D:\workspace_b\dev\engine.json D:\workspace_b\3rdParty\3rdParty.txt'),
        (r'D:\workspace_a\dev\engine_root.txt D:\workspace_a\dev\engine.json D:\workspace_a\3rdParty\3rdParty.txt additional text',
         r'D:\workspace_b\dev\engine_root.txt D:\workspace_b\dev\engine.json D:\workspace_b\3rdParty\3rdParty.txt additional text')
    ])
def test_replace_engine_path_and_tp_root_in_string_success(mocked_bld_a, string_a, mocked_bld_b, string_b):
    assert artifacts_cache.replace_engine_path_and_tp_root_in_string(mocked_bld_a, string_a) \
           == artifacts_cache.replace_engine_path_and_tp_root_in_string(mocked_bld_b, string_b)


@pytest.mark.parametrize(
    "table_a, vars_list_a, table_b, vars_list_b", [
        ({
            'CPPFLAGS': [
            ],
            'CCDEPS': [
            ],
            'FRAMEWORKPATH': [
            ],
            'LINKDEPS': [
            ],
            'INCLUDES': [
                r'.',
                r'D:\workspace_a\3rdParty\boost\1.61.0-az.4',
                r'D:\workspace_a\dev\Code\CryEngine\CryCommon',
                r'D:\workspace_a\3rdParty\rapidjson\rapidjson-1.0.2.1\include'
            ],
            'CXXFLAGS': [
                '/bigobj', '/nologo', '/WX', '/MP', '/Gy', '/GF', '/Gm-', '/fp:fast', '/Zc:wchar_t', '/W4', '/Z7'
            ],
            'INCPATHS': [
                r'D:\workspace_a\dev\BinTemp\win_x64_vs2017_profile\Code\Framework\AzCore\AzCore',
                r'D:\workspace_a\dev\Code\Framework\AzCore\AzCore',
                r'D:\workspace_a\3rdParty\boost\1.61.0-az.4',
                r'D:\workspace_a\dev\Code\CryEngine\CryCommon',
                r'D:\workspace_a\dev\BinTemp\win_x64_vs2017_profile\Code\Framework\AzCore',
                r'D:\workspace_a\dev\Code\Framework\AzCore',
                r'D:\workspace_a\3rdParty\rapidjson\rapidjson-1.0.2.1\include',
            ],
            'CFLAGS': [
                '/bigobj', '/nologo', '/W3', '/WX', '/MP', '/Gy', '/GF', '/Gm-', '/fp:fast', '/Zc:wchar_t', '/Zc:forScope', '/MD', '/Z7'
            ],
            'ARFLAGS': [
                '/NOLOGO', '/NOLOGO', '/MACHINE:X64'
            ],
            'ARCH': [
            ],
            'CXXDEPS': [
            ],
            'DEFINES': [
                '_WIN32', '_WIN64', '_PROFILE', 'PROFILE', 'NDEBUG', 'AZ_ENABLE_TRACING', 'AZ_ENABLE_DEBUG_TOOLS', 'SCRIPTCANVAS_ERRORS_ENABLED'
            ]
        },
        ['ARCH', 'ARCH_ST', 'CPPFLAGS', 'CPPPATH_ST', 'CXX', 'CXXDEPS', 'CXXFLAGS', 'CXX_SRC_F', 'CXX_TGT_F',
         'DEFINES', 'DEFINES_ST', 'FRAMEWORKPATH', 'FRAMEWORKPATH_ST', 'INCPATHS'],
        {
             'CPPFLAGS': [
             ],
             'CCDEPS': [
             ],
             'FRAMEWORKPATH': [
             ],
             'LINKDEPS': [
             ],
             'INCLUDES': [
                 r'.',
                 r'D:\workspace_b\3rdParty\boost\1.61.0-az.4',
                 r'D:\workspace_b\dev\Code\CryEngine\CryCommon',
                 r'D:\workspace_b\3rdParty\rapidjson\rapidjson-1.0.2.1\include'
             ],
             'CXXFLAGS': [
                 '/bigobj', '/nologo', '/WX', '/MP', '/Gy', '/GF', '/Gm-', '/fp:fast', '/Zc:wchar_t', '/W4', '/Z7'
             ],
             'INCPATHS': [
                 r'D:\workspace_b\dev\BinTemp\win_x64_vs2017_profile\Code\Framework\AzCore\AzCore',
                 r'D:\workspace_b\dev\Code\Framework\AzCore\AzCore',
                 r'D:\workspace_b\3rdParty\boost\1.61.0-az.4',
                 r'D:\workspace_b\dev\Code\CryEngine\CryCommon',
                 r'D:\workspace_b\dev\Code\Framework\AzCore',
                 r'D:\workspace_b\3rdParty\rapidjson\rapidjson-1.0.2.1\include',
             ],
             'CFLAGS': [
                 '/bigobj', '/nologo', '/W3', '/WX', '/MP', '/Gy', '/GF', '/Gm-', '/fp:fast', '/Zc:wchar_t',
                 '/Zc:forScope', '/MD', '/Z7'
             ],
             'ARFLAGS': [
                 '/NOLOGO', '/NOLOGO', '/MACHINE:X64'
             ],
             'ARCH': [
             ],
             'CXXDEPS': [
             ],
             'DEFINES': [
                 '_WIN32', '_WIN64', '_PROFILE', 'PROFILE', 'NDEBUG', 'AZ_ENABLE_TRACING', 'AZ_ENABLE_DEBUG_TOOLS',
                 'SCRIPTCANVAS_ERRORS_ENABLED'
             ]
         },
         ['ARCH', 'ARCH_ST', 'CPPFLAGS', 'CPPPATH_ST', 'CXX', 'CXXDEPS', 'CXXFLAGS', 'CXX_SRC_F', 'CXX_TGT_F',
          'DEFINES', 'DEFINES_ST', 'FRAMEWORKPATH', 'FRAMEWORKPATH_ST', 'INCPATHS']
        )
    ])
def test_hash_env_vars_success(mocked_bld_a, mocked_env_a, vars_list_a, mocked_bld_b, mocked_env_b, vars_list_b):
    assert artifacts_cache.hash_env_vars(mocked_bld_a, mocked_env_a, vars_list_a) == artifacts_cache.hash_env_vars(mocked_bld_b, mocked_env_b, vars_list_b)
    


def test_azcg_uid():
    
    class fake_self(object):
        
        def __init__(self):

            self.bld = mock.Mock()
            self.bld.configure_mock(**{'engine_path_replaced': 'yes',
                                       'tp_root_replaced': 'yes'})

            self.path = mock.Mock()
            self.path.configure_mock(**{'abspath.return_value': 'fake_path'})

            self.input_dir = mock.Mock()
            self.input_dir.configure_mock(**{'abspath.return_value': 'fake_input_dir'})

            self.output_dir = mock.Mock()
            self.output_dir.configure_mock(**{'abspath.return_value': 'fake_output_dir'})

            self.inputs = ['fake_input1', 'fake_input2']
            self.defines = ['fake_define1', 'fake_define2']
            self.argument_list = ['fake_arg1', 'fake_arg2']
            self.azcg_deps = ['fake_deps1', 'fake_deps2']
    
    result = artifacts_cache.azcg_uid(fake_self())


    pass
