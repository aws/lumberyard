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
import warnings

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
    cf_key_name = 'pk-MOCKCFKEY.pem'

    def setUp(self):
        self.prepare_test_environment("dynamic_content_tests")
        # Ignore warnings based on https://github.com/boto/boto3/issues/454 for now
        # Needs to be set per tests as its reset between intergration tests
        warnings.filterwarnings(action="ignore", message="unclosed", category=ResourceWarning)
        self.register_for_shared_resources()
        self.enable_shared_gem(self.gem_name)

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_setup_base_stack(self):
        self.setup_base_stack()

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
        self.lmbr_aws('dynamic-content', 'upload-manifest-content','--manifest-path',self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.lmbr_aws('dynamic-content', 'list-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn(self.manifest_pak_name, self.lmbr_aws_stdout)
        self.assertIn(self.content_pak_name, self.lmbr_aws_stdout)
        self.assertIn(self.manifest_name, self.lmbr_aws_stdout)

    def __110_compare_bucket_content_match(self):
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __115_update_local_content(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT2')
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __120_upload_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content','--manifest-path',self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __300_delete_content(self):
        self.lmbr_aws('dynamic-content', 'clear-dynamic-content','--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('Key not found', self.lmbr_aws_stdout)

    def __310_delete_deployment_stack_without_deployment_tags(self):
        self.unregister_for_shared_resources()
        self.runtest(self.base_delete_deployment_stack, name='base_delete_deployment_stack_without_deployment_tags')

    def __400_create_deployment_stack_with_deployment_tags(self):
        self.set_deployment_tags('content-distribution')
        self.runtest(self.base_create_deployment_stack, name='base_create_deployment_stack_with_deployment_tags')

    def __410_upload_cf_key(self):
        self.make_temp_asset_file(self.cf_key_name, 'FAKECFKEY')
        self.lmbr_aws('dynamic-content', 'upload-cf-key', '--key-path', os.path.join(self.PC_CACHE_DIR, self.cf_key_name), '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.assertIn('Uploaded key file', self.lmbr_aws_stdout)

    def __420_upload_monitored_content_and_invalidate_existing_files(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT1')
        self.lmbr_aws('dynamic-content', 'add-manifest-file', '--file-name', self.monitored_content_name, '--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'add-pak', '--pak-name', self.content_pak_name, '--manifest-path',self.manifest_name, '--platform-type', self.content_pak_platform)
        self.lmbr_aws('dynamic-content', 'add-file-to-pak', '--pak-file', self.content_pak_file, '--manifest-path',self.manifest_name, '--file-name', self.monitored_content_name, '--platform-type', self.content_file_platform)
        self.lmbr_aws('dynamic-content', 'upload-manifest-content','--manifest-path',self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)

        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT2')
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'upload-manifest-content','--manifest-path',self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME, '--invalidate-existing-files')
        self.removed_from_edge_cache(self.manifest_name)
        self.removed_from_edge_cache(self.manifest_pak_name)
        self.removed_from_edge_cache(self.content_pak_file)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __430_upload_monitored_content_without_invalidating_existing_files(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT3')
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path',self.manifest_name)
        self.lmbr_aws('dynamic-content', 'upload-manifest-content','--manifest-path',self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.removed_from_edge_cache(self.manifest_name, False)
        self.removed_from_edge_cache(self.manifest_pak_name, False)
        self.removed_from_edge_cache(self.content_pak_file, False)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path',self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __440_invalidate_file(self):
        self.lmbr_aws('dynamic-content', 'invalidate-file','--file-path',self.content_pak_file, '--caller-reference', 'cctest')
        self.removed_from_edge_cache(self.content_pak_file)

    def __999_teardown_base_stack(self):
        self.teardown_base_stack()

    def make_temp_asset_file(self,temp_file_name, temp_file_content):
        cache_dir = self.PC_CACHE_DIR
        if not os.path.exists(cache_dir):
            os.makedirs(cache_dir)
        output_path = os.path.join(cache_dir, temp_file_name)
        print('writing {} to {}'.format(temp_file_content, output_path))
        with open(output_path, 'w') as f:
            f.write(temp_file_content)

    def removed_from_edge_cache(self, file_name, expect_in_log=True):
        log_content = f"File /{file_name} is removed from CloudFront edge caches."
        if expect_in_log:
            self.assertIn(log_content, self.lmbr_aws_stdout)
        else:
            self.assertNotIn(log_content, self.lmbr_aws_stdout)
