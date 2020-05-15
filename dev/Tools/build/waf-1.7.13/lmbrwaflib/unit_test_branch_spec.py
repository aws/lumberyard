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

# waflib imports
from waflib import Context

# misc imports
from waf_branch_spec import BINTEMP_FOLDER


class FakeGModule(object):
    pass
Context.g_module = FakeGModule
from lmbrwaflib import branch_spec


class FakeContextBranchSpec(object):
    pass


class FakeBldNode(object):
    
    def __init__(self, name):
        self.name = name
        
    def __str__(self):
        return self.name
    
    def make_node(self, name):
        child = FakeBldNode(name)
        child.parent = self
        return child


@pytest.fixture
def fake_waf_context(fake_ctx_type):
    
    ctx = FakeContextBranchSpec()
    if fake_ctx_type == 'configure':
        setattr(ctx, 'path', FakeBldNode('dev'))

    elif fake_ctx_type == 'project_generator':
        setattr(ctx, 'bldnode', FakeBldNode(BINTEMP_FOLDER))
        
    elif fake_ctx_type == 'build':
        temp_dir_node = FakeBldNode(BINTEMP_FOLDER)
        setattr(ctx, 'bldnode', temp_dir_node.make_node('build_variant'))
        
    else:
        assert False
        
    return ctx


@pytest.mark.parametrize(
    "fake_ctx_type", [
        pytest.param('configure'),
        pytest.param('project_generator'),
        pytest.param('build')
    ]
)
def test_get_bintemp_folder_node(fake_waf_context, fake_ctx_type):
    
    result = branch_spec.get_bintemp_folder_node(fake_waf_context)
    assert str(result) == BINTEMP_FOLDER

def test_update_environment_from_bootstrap_params():
    environment = {}
    # test data is tuples of (input command line, expected output env dictionary)
    test_data = [
        ('', {}),
        ('--enablecapability compilegame ', {}  # only compileandroid matters to android.
        ), 
        # now test each param on its own:
        (
            '--jdk="C:\\Program Files\\Java\\jdk1.8.0_231"', {
            'LY_JDK' : 'C:\\Program Files\\Java\\jdk1.8.0_231' # with spaces in it
            }
        ),
        (
            '--android-sdk="C:\\A n d r o i d\\android-sdk"', {
            'LY_ANDROID_SDK' : 'C:\\A n d r o i d\\android-sdk' # with spaces in it
            }
        ),
        (
            '--android-ndk="C:\\A n d r o i d\\android-ndk"', {
            'LY_ANDROID_NDK' : 'C:\\A n d r o i d\\android-ndk' # with spaces in it
            }
        ),
        #now without spaces and in other param forms
        (
            '--jdk="C:\\quotes_but_no_spaces\\jdk"', { # still with quotes but no spaces
            'LY_JDK' : 'C:\\quotes_but_no_spaces\\jdk'
            }
        ),
        (
            # here we prove that we do not use unix backslash escaping rules
            # normally, under unix rules you would need to double escape the slash (so \\\\)
            # but for backward compat, we don't want to require that
            '--jdk=C:\\no_quotes_no_spaces\\jdk', { # no quotes, no spaces 
            'LY_JDK' : 'C:\\no_quotes_no_spaces\\jdk'
            }
        ),
        (
            # normally, under unix rules you would need to double escape the slash (so \\\\)
            # but for backward compat, we don't want to require that
            '--jdk C:\\nothing_special_no_equals\\jdk', { # no quotes, no spaces, no equals
            'LY_JDK' : 'C:\\nothing_special_no_equals\\jdk'
            }
        ),
        (
            '--jdk "C:\\Program Files\\Java\\jdk1.8.0_231"', {
            'LY_JDK' : 'C:\\Program Files\\Java\\jdk1.8.0_231' # with spaces in it but no equals
            }
        ),
        # test android enabling
        (
            '--enablecapability compileandroid', {
            'ENABLE_ANDROID' : 'True' # this is not a typo, the code expects a string value like 'True' or 'False'
            }
        ),
        (
            '--enablecapability whatever', {} # other capabilites do not set android to true
        ),
        #now test them all at once!
        (
            ' --enablecapability compileandroid --enablecapability compilegame' \
            ' --jdk="C:\\Program Files\\Java\\jdk1.8.0_231"' \
            ' --android-ndk="C:\\A n d r o i d\\android-ndk"' \
            ' --android-sdk="C:\\A n d r o i d\\android-sdk"'
            , 
            {
                'ENABLE_ANDROID' : 'True',
                'LY_ANDROID_NDK' : 'C:\\A n d r o i d\\android-ndk',
                'LY_ANDROID_SDK' : 'C:\\A n d r o i d\\android-sdk',
                'LY_JDK' : 'C:\\Program Files\\Java\\jdk1.8.0_231',
            }
        ),
    ]
    
    for input, expected_output in test_data:
        actual_output = {}
        branch_spec.update_android_environment_from_bootstrap_params(actual_output, input)
        assert actual_output == expected_output
