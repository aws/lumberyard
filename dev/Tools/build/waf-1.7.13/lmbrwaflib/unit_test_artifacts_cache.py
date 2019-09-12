import pytest
from mock import MagicMock
from waflib.ConfigSet import ConfigSet
import artifacts_cache


@pytest.fixture()
def mocked_bld_a():
    bld = MagicMock()
    tp = MagicMock(content={'3rdPartyRoot': r'D:\workspace_a\3rdParty'})
    bld.configure_mock(engine_path=r'D:\workspace_a\dev', tp=tp)
    del bld.engine_path_replaced
    del bld.tp_root_replaced
    del bld.cache_env
    return bld


@pytest.fixture()
def mocked_bld_b():
    bld = MagicMock()
    tp = MagicMock(content={'3rdPartyRoot': r'D:\workspace_b\3rdParty'})
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
                r'D:\workspace_a\3rdParty\boost\1.61.0-az.2',
                r'D:\workspace_a\dev\Code\CryEngine\CryCommon',
                r'D:\workspace_a\3rdParty\rapidjson\rapidjson-1.0.2.1\include'
            ],
            'CXXFLAGS': [
                '/bigobj', '/nologo', '/WX', '/MP', '/Gy', '/GF', '/Gm-', '/fp:fast', '/Zc:wchar_t', '/W4', '/Z7'
            ],
            'INCPATHS': [
                r'D:\workspace_a\dev\BinTemp\win_x64_vs2015_profile\Code\Framework\AzCore\AzCore',
                r'D:\workspace_a\dev\Code\Framework\AzCore\AzCore',
                r'D:\workspace_a\3rdParty\boost\1.61.0-az.2',
                r'D:\workspace_a\dev\Code\CryEngine\CryCommon',
                r'D:\workspace_a\dev\BinTemp\win_x64_vs2015_profile\Code\Framework\AzCore',
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
                 r'D:\workspace_b\3rdParty\boost\1.61.0-az.2',
                 r'D:\workspace_b\dev\Code\CryEngine\CryCommon',
                 r'D:\workspace_b\3rdParty\rapidjson\rapidjson-1.0.2.1\include'
             ],
             'CXXFLAGS': [
                 '/bigobj', '/nologo', '/WX', '/MP', '/Gy', '/GF', '/Gm-', '/fp:fast', '/Zc:wchar_t', '/W4', '/Z7'
             ],
             'INCPATHS': [
                 r'D:\workspace_b\dev\BinTemp\win_x64_vs2015_profile\Code\Framework\AzCore\AzCore',
                 r'D:\workspace_b\dev\Code\Framework\AzCore\AzCore',
                 r'D:\workspace_b\3rdParty\boost\1.61.0-az.2',
                 r'D:\workspace_b\dev\Code\CryEngine\CryCommon',
                 r'D:\workspace_b\dev\BinTemp\win_x64_vs2015_profile\Code\Framework\AzCore',
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