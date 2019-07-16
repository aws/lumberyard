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

from waflib import Logs
import pytest

import unit_test
import packaging
import shutil


@pytest.fixture()
def test_context(tmpdir):
    fake_context = unit_test.FakeContext(str(tmpdir.realpath()), False)
    return fake_context


def test_PackageContextClearUserCache_Success(tmpdir, test_context):
    
    tmpdir.ensure('dev/cache/game/osx_gl/user', dir=True)
    asset_cache_node = test_context.srcnode.make_node(['cache','game','osx_gl'])
    packaging.clear_user_cache(asset_cache_node)
    

def test_PackageContextClearUserCache_IgnoreNoNode(tmpdir, test_context):
    
    packaging.clear_user_cache(None)

    

def test_PackageContextClearUserCache_IgnorePathDoesNotExist(tmpdir, test_context):
    
    tmpdir.ensure('dev/cache', dir=True)
    asset_cache_node = test_context.srcnode.make_node(['cache','game','osx_gl'])
    packaging.clear_user_cache(asset_cache_node)


def test_PackageContextClearUserCache_Warn(tmpdir, test_context):
    
    old_rmtree = shutil.rmtree
    old_warn = Logs.warn
    test_context.warned = False
    try:
        def _stub_error_rmtree(path):
            raise OSError("Error")
        shutil.rmtree = _stub_error_rmtree

        def _stub_log_warn(msg):
            test_context.warned = True

        Logs.warn = _stub_log_warn
        
        tmpdir.ensure('dev/cache/game/osx_gl/user', dir=True)
        asset_cache_node = test_context.srcnode.make_node(['cache','game','osx_gl'])
        packaging.clear_user_cache(asset_cache_node)
        
        assert test_context.warned
    finally:
        shutil.rmtree = old_rmtree
        Logs.warn = old_warn
    
