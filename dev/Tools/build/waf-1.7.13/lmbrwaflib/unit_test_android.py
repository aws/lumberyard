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

from waflib import Errors, Logs

import unit_test
import pytest

import android
import packaging
import shutil


@pytest.fixture
def fake_ctx(tmpdir):
    fake_context = unit_test.FakeContext(str(tmpdir.realpath()), False)
    tmpdir.ensure('dev/layout', dir=True)
    fake_context.layout_node = fake_context.srcnode.make_node('layout')
    tmpdir.ensure('dev/shaders', dir=True)
    # Dir has been created, set cache_isdir to avoid calling mkdir on this node, the mkdir code
    # doesn't seem to like the structure of this fake context.
    fake_context.layout_node.cache_isdir = True
    fake_context.shaders_node = fake_context.srcnode.make_node('shaders')
    tmpdir.ensure('dev/assets_cache', dir=True)
    fake_context.assets_cache = fake_context.srcnode.make_node('assets_cache')
    tmpdir.ensure('dev/engine', dir=True)
    fake_context.engine_node = fake_context.srcnode.make_node('engine')
    return fake_context


@pytest.mark.parametrize(
    "game, assets_type", [
        pytest.param('game','es3')
    ]
)
def test_BuildShaderPaks_NoShaderPaks_Error(tmpdir, fake_ctx, game, assets_type):
    
    Logs.init_log()
    
    def _mock_generate_shaders_pak(ctx, game, assets_platform, shader_type):
        return fake_ctx.shaders_node
    old_generate_shaders_pak = packaging.generate_shaders_pak
    packaging.generate_shaders_pak = _mock_generate_shaders_pak
    try:
        android.build_shaders(fake_ctx, game, assets_type, fake_ctx.layout_node, fake_ctx.assets_cache, generate_pak=False)
        assert False, "Error expected"
    except Errors.WafError:
        pass
    finally:
        packaging.generate_shaders_pak = old_generate_shaders_pak


@pytest.mark.parametrize(
    "game, assets_type", [
        pytest.param('game', 'es3')
    ]
)
def test_BuildShaderPaks_NoShaderExistingLayoutPaks_Success(tmpdir, fake_ctx, game, assets_type):
    
    tmpdir.join('dev/shaders/shaders.pak').write("pak")

    def _mock_generate_shaders_pak(ctx, game, assets_platform, shader_type):
        return fake_ctx.shaders_node
    old_generate_shaders_pak = packaging.generate_shaders_pak
    packaging.generate_shaders_pak = _mock_generate_shaders_pak

    def _mock_copy2(src, dst):
        pass
    old_copy2 = shutil.copy2
    shutil.copy2 = _mock_copy2
    try:
        android.build_shaders(fake_ctx, game, assets_type, fake_ctx.layout_node, fake_ctx.assets_cache, generate_pak=True)
    finally:
        packaging.generate_shaders_pak = old_generate_shaders_pak
        shutil.copy2 = old_copy2


@pytest.mark.parametrize(
    "game, assets_type", [
        pytest.param('game', 'es3')
    ]
)
def test_BuildShaderPaks_ShaderExistingLayoutPaks_Success(tmpdir, fake_ctx, game, assets_type):
    tmpdir.join('dev/shaders/shaders.pak').write("pak")
    tmpdir.ensure('dev/layout/{}'.format(game.lower()), dir=True)
    tmpdir.join('dev/layout/{}/shaders.pak'.format(game.lower())).write('pak')
    
    def _mock_generate_shaders_pak(ctx, game, assets_platform, shader_type):
        return fake_ctx.shaders_node
    
    old_generate_shaders_pak = packaging.generate_shaders_pak
    packaging.generate_shaders_pak = _mock_generate_shaders_pak
    
    def _mock_copy2(src, dst):
        pass
    
    old_copy2 = shutil.copy2
    shutil.copy2 = _mock_copy2
    try:
        android.build_shaders(fake_ctx, game, assets_type, fake_ctx.layout_node, fake_ctx.assets_cache, generate_pak=True)
    finally:
        packaging.generate_shaders_pak = old_generate_shaders_pak
        shutil.copy2 = old_copy2
