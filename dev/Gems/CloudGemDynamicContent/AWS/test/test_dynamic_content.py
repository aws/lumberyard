#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the 'License'). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# $Revision: #1 $

import os
import json
import sys
import unittest

from botocore.exceptions import ClientError

from resource_manager.test import lmbr_aws_test_support
from resource_manager.test import base_stack_test

class IntegrationTest_CloudGemDynamicContent_EndToEnd(base_stack_test.BaseStackTestCase):
    gem_name = 'CloudGemDynamicContent'

    manifest_name = 'temp_manifest.json'
    monitored_content_name = 'some_temp_file.txt'
    manifest_pak_name = 'temp_manifest.manifest.pak'
    content_pak_name = 'temp_pak'
    content_pak_platform = 'shared'
    content_file_platform = ''
    content_pak_file = content_pak_name + os.path.extsep + content_pak_platform + os.path.extsep + 'pak'

    pak_tool_name = '7za.exe'

    def setUp(self):
        self.prepare_test_envionment("dynamic_content_tests")

        tools_dir = os.path.join('Tools')
        pak_tool_path = os.path.join(tools_dir, self.pak_tool_name)
        # Copy our pak tool
        self.copy_support_file(pak_tool_path)

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_setup_base_stack(self):
        self.setup_base_stack()

    def __080_add_gem_resource_group(self):
        self.enable_real_gem(self.gem_name)

    def __085_upload_gem_resources(self):
        self.lmbr_aws('upload-resources', '--deployment', self.TEST_DEPLOYMENT_NAME, '--resource-group', self.gem_name, '--confirm-aws-usage', '--confirm-security-change')
        self.lmbr_aws('list-resource-groups', '--deployment', self.TEST_DEPLOYMENT_NAME)
        self.assertIn(self.gem_name, self.lmbr_aws_stdout)
        self.assertIn('CREATE_COMPLETE', self.lmbr_aws_stdout)

    def make_temp_asset_file(self,temp_file_name, temp_file_content):
        cache_dir = self.PC_CACHE_DIR
        if not os.path.exists(cache_dir):
            os.mkdir(cache_dir)
        output_path = os.path.join(cache_dir, temp_file_name)
        print 'writing ' + temp_file_content + ' to '+  output_path
        with open(output_path, 'w') as f:
            f.write(temp_file_content)

    def __090_add_monitored_content(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT1')
        self.lmbr_aws('dynamic-content', 'add-manifest-file', '--file-name', self.monitored_content_name, '--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path',self.manifest_name)
        self.assertIn(self.monitored_content_name, self.lmbr_aws_stdout)

    def __091_add_pak_to_manifest(self):
        self.lmbr_aws('dynamic-content', 'add-pak', '--pak-name', self.content_pak_name, '--manifest-path',self.manifest_name, '--platform-type', self.content_pak_platform)
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path',self.manifest_name)
        self.assertIn(self.content_pak_file, self.lmbr_aws_stdout)

    def __092_add_file_to_pak(self):
        self.lmbr_aws('dynamic-content', 'add-file-to-pak', '--pak-file', self.content_pak_file, '--manifest-path',self.manifest_name, '--file-name', self.monitored_content_name, '--platform-type', self.content_file_platform)
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path',self.manifest_name, '--file-name', self.monitored_content_name, '--platform-type', self.content_file_platform)
        self.assertIn(self.content_pak_file, self.lmbr_aws_stdout)

    def __095_compare_bucket_content_new(self):
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('Key not found', self.lmbr_aws_stdout)

    def __100_list_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'list-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('New Local Key', self.lmbr_aws_stdout)

    def __105_upload_monitored_content(self):

        self.lmbr_aws('dynamic-content', 'upload-manifest-content','--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'list-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn(self.manifest_pak_name, self.lmbr_aws_stdout)
        self.assertIn(self.content_pak_name, self.lmbr_aws_stdout)

    def __110_compare_bucket_content_match(self):
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __115_update_local_content(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT2')
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __120_upload_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content','--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __130_delete_content(self):
        self.lmbr_aws('dynamic-content', 'clear-dynamic-content','--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('Key not found', self.lmbr_aws_stdout)

    def __999_teardown_base_stack(self):
        self.teardown_base_stack()
