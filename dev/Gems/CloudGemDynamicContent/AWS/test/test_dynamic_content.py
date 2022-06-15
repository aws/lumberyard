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

import json
import os
import warnings
import zipfile

from resource_manager.test import base_stack_test


class IntegrationTest_CloudGemDynamicContent_EndToEnd(base_stack_test.BaseStackTestCase):
    gem_name = 'CloudGemDynamicContent'

    manifest_name = 'temp_manifest.json'
    monitored_content_name = 'some_temp_file.txt'
    monitored_folder = 'test'
    manifest_pak_name = 'temp_manifest.manifest.pak'
    content_pak_name = 'temp_pak'
    content_pak_platform = 'shared'
    content_file_platform = ''
    content_pak_file = content_pak_name + os.path.extsep + content_pak_platform + os.path.extsep + 'pak'
    cf_key_name = 'pk-MOCKCFKEY.pem'

    def setUp(self):
        self.prepare_test_environment("dynamic_content_tests")
        # Ignore warnings based on https://github.com/boto/boto3/issues/454 for now
        # Needs to be set per tests as its reset between integration tests
        warnings.filterwarnings(action="ignore", message="unclosed", category=ResourceWarning)
        self.register_for_shared_resources()
        self.enable_shared_gem(self.gem_name)

    def test_end_to_end(self):
        self.run_all_tests()

    def __000_setup_base_stack(self):
        self.setup_base_stack()

    def __090_add_monitored_content(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT1')
        self.lmbr_aws('dynamic-content', 'add-manifest-file', '--file-name', self.monitored_content_name, '--manifest-path', self.manifest_name)
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path', self.manifest_name)
        self.assertIn(self.monitored_content_name, self.lmbr_aws_stdout)

    def __091_add_pak_to_manifest(self):
        self.lmbr_aws('dynamic-content', 'add-pak', '--pak-name', self.content_pak_name, '--manifest-path', self.manifest_name, '--platform-type',
                      self.content_pak_platform)
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path', self.manifest_name)
        self.assertIn(self.content_pak_file, self.lmbr_aws_stdout)

    def __092_add_file_to_pak(self):
        self.lmbr_aws('dynamic-content', 'add-file-to-pak', '--pak-file', self.content_pak_file, '--manifest-path', self.manifest_name, '--file-name',
                      self.monitored_content_name, '--platform-type', self.content_file_platform)
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path', self.manifest_name, '--file-name', self.monitored_content_name, '--platform-type',
                      self.content_file_platform)
        self.assertIn(self.content_pak_file, self.lmbr_aws_stdout)

    def __095_compare_bucket_content_new(self):
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('Key not found', self.lmbr_aws_stdout)

    def __100_list_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'list-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('New Local Key', self.lmbr_aws_stdout)

    def __101_list_uploaded_content(self):
        """Test the uploaded content can be described"""
        self.lmbr_aws('dynamic-content', 'list-uploaded-content')
        self.assertIn('default.json PUBLIC', self.lmbr_aws_stdout)

    def __105_upload_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.lmbr_aws('dynamic-content', 'list-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn(self.manifest_pak_name, self.lmbr_aws_stdout)
        self.assertIn(self.content_pak_name, self.lmbr_aws_stdout)
        self.assertIn(self.manifest_name, self.lmbr_aws_stdout)

    def __110_compare_bucket_content_match(self):
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __115_update_local_content(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT2')
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path', self.manifest_name)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __120_upload_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __130_rebuild_paks(self):
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path', self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertNotIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __131_rebuild_paks_regardless_of_file_change(self):
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path', self.manifest_name, '--all')
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertNotIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __132_upload_monitored_content(self):
        # Verify that paks with same content will not be uploaded again
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertNotIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __133_upload_monitored_content_regardless_of_file_change(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME,
                      '--all')
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertNotIn('UPDATE Manifest', self.lmbr_aws_stdout)
        self.assertIn(f'Found updated item {self.content_pak_file}', self.lmbr_aws_stdout)

    def __135_list_uploaded_content_changed(self):
        """Test the uploaded content can be described"""
        self.lmbr_aws('dynamic-content', 'list-uploaded-content')
        self.assertIn(f'{self.manifest_name} PRIVATE', self.lmbr_aws_stdout)

    def __200_delete_content(self):
        self.lmbr_aws('dynamic-content', 'clear-dynamic-content')
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('Key not found', self.lmbr_aws_stdout)

    def __210_upload_folder_with_same_content(self):
        # Create a temp folder which contains a test pak file
        temp_asset_dir = self.make_temp_asset_folder(self.monitored_folder, self.content_pak_file, self.monitored_content_name, 'TESTCONTENT')
        self.lmbr_aws('dynamic-content', 'upload-folder', '--folder', temp_asset_dir, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.assertIn(f'Key not found {self.content_pak_file}', self.lmbr_aws_stdout)
        self.assertIn('Uploading', self.lmbr_aws_stdout)
        # Remove and regenerate the test pak file with the same content
        os.remove(os.path.join(temp_asset_dir, self.content_pak_file))
        self.assertTrue(len(os.listdir(temp_asset_dir)) == 0)
        self.make_temp_asset_folder(self.monitored_folder, self.content_pak_file, self.monitored_content_name, 'TESTCONTENT')
        # Try to upload the binary-identical asset again
        self.lmbr_aws('dynamic-content', 'upload-folder', '--folder', temp_asset_dir, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        # Verify that the pak without any content change isn't uploaded to the content bucket
        self.assertIn('Bucket entry matches, skipping', self.lmbr_aws_stdout)

    def __220_upload_folder_with_new_content(self):
        temp_asset_dir = self.make_temp_asset_folder(self.monitored_folder, self.content_pak_file, self.monitored_content_name, 'TESTCONTENT3')
        self.lmbr_aws('dynamic-content', 'upload-folder', '--folder', temp_asset_dir, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.assertIn(f'Uploading', self.lmbr_aws_stdout)

    def __250_can_preview_delete_uploaded_content(self):
        temp_asset_dir = self.make_temp_asset_folder(self.monitored_folder, self.content_pak_file, self.monitored_content_name, 'TESTCONTENT3')
        self.lmbr_aws('dynamic-content', 'upload-folder', '--folder', temp_asset_dir, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.lmbr_aws('dynamic-content', 'delete-uploaded-content', '--file-path', self.content_pak_file)
        self.assertIn(f'{self.content_pak_file}', self.lmbr_aws_stdout)
        self.assertIn('would be deleted', self.lmbr_aws_stdout)

    def __255_can_delete_uploaded_content(self):
        temp_asset_dir = self.make_temp_asset_folder(self.monitored_folder, self.content_pak_file, self.monitored_content_name, 'TESTCONTENT3')
        self.lmbr_aws('dynamic-content', 'upload-folder', '--folder', temp_asset_dir, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.lmbr_aws('dynamic-content', 'delete-uploaded-content', '--file-path', self.content_pak_file, '--confirm-deletion')
        self.assertIn(f"'DeletedFileList': ['{self.content_pak_file}']", self.lmbr_aws_stdout)

    def __300_delete_base_deployment_stack_without_deployment_tags(self):
        self.unregister_for_shared_resources()
        self.runtest(self.base_delete_deployment_stack, name='base_delete_deployment_stack_without_deployment_tags')

    def __400_create_deployment_stack_with_content_distribution_deployment_tag(self):
        self.set_deployment_tags('content-distribution')
        self.runtest(self.base_create_deployment_stack, name='base_create_deployment_stack_with_content_distribution_deployment_tag')

    def __410_upload_cf_key(self):
        self.make_temp_asset_file(self.cf_key_name, 'FAKECFKEY')
        self.lmbr_aws('dynamic-content', 'upload-cf-key', '--key-path', os.path.join(self.PC_CACHE_DIR, self.cf_key_name), '--deployment-name',
                      self.TEST_DEPLOYMENT_NAME)
        self.assertIn('Uploaded key file', self.lmbr_aws_stdout)

    def __420_upload_monitored_content_and_invalidate_existing_files(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT1')
        self.lmbr_aws('dynamic-content', 'add-manifest-file', '--file-name', self.monitored_content_name, '--manifest-path', self.manifest_name)
        self.lmbr_aws('dynamic-content', 'add-pak', '--pak-name', self.content_pak_name, '--manifest-path', self.manifest_name, '--platform-type',
                      self.content_pak_platform)
        self.lmbr_aws('dynamic-content', 'add-file-to-pak', '--pak-file', self.content_pak_file, '--manifest-path', self.manifest_name, '--file-name',
                      self.monitored_content_name, '--platform-type', self.content_file_platform)
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)

        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT2')
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path', self.manifest_name)
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME,
                      '--invalidate-existing-files')
        self.removed_from_edge_cache(self.manifest_name)
        self.removed_from_edge_cache(self.manifest_pak_name)
        self.removed_from_edge_cache(self.content_pak_file)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __430_upload_monitored_content_without_invalidating_existing_files(self):
        self.make_temp_asset_file(self.monitored_content_name, 'TESTCONTENT4')
        self.lmbr_aws('dynamic-content', 'build-new-paks', '--manifest-path', self.manifest_name)
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.removed_from_edge_cache(self.manifest_name, False)
        self.removed_from_edge_cache(self.manifest_pak_name, False)
        self.removed_from_edge_cache(self.content_pak_file, False)
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)

    def __440_invalidate_file(self):
        self.lmbr_aws('dynamic-content', 'invalidate-file', '--file-path', self.content_pak_file, '--caller-reference', 'cctest')
        self.removed_from_edge_cache(self.content_pak_file)

    def __500_enable_content_versioning(self):
        self.lmbr_aws('deployment', 'tags', '--add', 'content-versioning')
        self.lmbr_aws('deployment', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change',
                      '--confirm-resource-deletion')
        self.assertIn('Found 5 table entries', self.lmbr_aws_stdout)
        self.assertIn('Migrated table entry', self.lmbr_aws_stdout)

    def __510_upload_new_version_of_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME,
                      '--all')
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __520_list_manifest_versions(self):
        self.lmbr_aws('dynamic-content', 'list-file-versions', '--file-name', self.manifest_name)
        self.assertIn(f'File {self.manifest_name} has 2 version(s)', self.lmbr_aws_stdout)
        self.assertIn('Version ID', self.lmbr_aws_stdout)
        self.assertIn('Last Modified', self.lmbr_aws_stdout)
        self.assertIn('(Latest)', self.lmbr_aws_stdout)

    def __530_show_manifest(self):
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path', self.manifest_name)
        self.assertIn('Version ID', self.lmbr_aws_stdout)
        self.lmbr_aws('dynamic-content', 'show-manifest', '--manifest-path', self.manifest_name, '--manifest-version-id', 'null')
        self.assertNotIn('Version ID', self.lmbr_aws_stdout)

    def __540_list_bucket_versions(self):
        self.lmbr_aws('dynamic-content', 'list-bucket-content', '--manifest-path', self.manifest_name, '--manifest-version-id', 'null')
        self.assertIn('Comparing versions', self.lmbr_aws_stdout)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __550_compare_bucket_versions(self):
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name, '--manifest-version-id', 'null')
        self.assertIn('Comparing versions', self.lmbr_aws_stdout)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)

    def __560_replace_old_monitored_content_versions(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME,
                      '--all', '--replace')
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)
        self.lmbr_aws('dynamic-content', 'list-file-versions', '--file-name', self.manifest_name)
        self.assertIn(f'File {self.manifest_name} has 1 version(s)', self.lmbr_aws_stdout)

    def __570_upload_new_version_of_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME,
                      '--all')
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)
        self.lmbr_aws('dynamic-content', 'list-file-versions', '--file-name', self.manifest_name)
        self.assertIn(f'File {self.manifest_name} has 2 version(s)', self.lmbr_aws_stdout)

    def __580_remove_noncurrent_versions(self):
        self.lmbr_aws('dynamic-content', 'clear-dynamic-content', '--noncurrent-versions', '-C')
        self.lmbr_aws('dynamic-content', 'list-file-versions', '--file-name', self.manifest_name)
        self.assertIn(f'File {self.manifest_name} has 1 version(s)', self.lmbr_aws_stdout)
        self.assertIn('(Latest)', self.lmbr_aws_stdout)

    def __590_upload_new_version_of_monitored_content(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME,
                      '--all')
        self.assertIn('MATCH Manifest', self.lmbr_aws_stdout)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)
        self.lmbr_aws('dynamic-content', 'list-file-versions', '--file-name', self.manifest_name)
        self.assertIn(f'File {self.manifest_name} has 2 version(s)', self.lmbr_aws_stdout)

    def __600_suspend_content_versioning(self):
        self.lmbr_aws('dynamic-content', 'suspend-versioning', '-C')
        self.lmbr_aws('deployment', 'update', '-d', self.TEST_DEPLOYMENT_NAME, '--confirm-aws-usage', '--confirm-security-change',
                      '--confirm-resource-deletion')
        self.assertIn('Found 8 table entries', self.lmbr_aws_stdout)
        self.assertIn('Migrated table entry', self.lmbr_aws_stdout)

    def __610_validate_manifest_after_suspend_versioning(self):
        self.lmbr_aws('dynamic-content', 'upload-manifest-content', '--manifest-path', self.manifest_name, '--deployment-name', self.TEST_DEPLOYMENT_NAME)
        self.assertIn('UPDATE Manifest', self.lmbr_aws_stdout)

        manifest_path = os.path.join(self.GAME_DIR, 'DynamicContent', 'Manifests', self.manifest_name)
        with open(manifest_path) as manifest_file:
            manifest = json.load(manifest_file)

        paks = manifest.get('Paks', [])
        # Manifest defines one pak file
        self.assertTrue(len(paks) == 1)
        for pak_entry in paks:
            # Manifest doesn't contain any version information about the pak file
            self.assertEqual(pak_entry.get('versionId'), None)

    def __620_delete_all_versions(self):
        self.lmbr_aws('dynamic-content', 'clear-dynamic-content', '--all-versions')
        self.lmbr_aws('dynamic-content', 'compare-bucket-content', '--manifest-path', self.manifest_name)
        self.assertIn('Key not found', self.lmbr_aws_stdout)

    def __999_teardown_base_stack(self):
        self.teardown_base_stack()

    def make_temp_asset_file(self, temp_file_name, temp_file_content):
        cache_dir = self.PC_CACHE_DIR
        output_path = os.path.join(cache_dir, temp_file_name)
        output_dir = os.path.dirname(output_path)
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        print('writing {} to {}'.format(temp_file_content, output_path))
        with open(output_path, 'w') as f:
            f.write(temp_file_content)

        return output_path

    def make_temp_asset_folder(self, temp_folder_name, temp_pak_name, temp_file_name, temp_file_content):
        temp_asset_path = self.make_temp_asset_file(os.path.join(temp_folder_name, temp_file_name), temp_file_content)
        temp_asset_dir = os.path.dirname(temp_asset_path)
        with zipfile.ZipFile(os.path.join(temp_asset_dir, temp_pak_name), 'w') as myzip:
            myzip.write(temp_asset_path, temp_file_name)
        os.remove(temp_asset_path)

        return temp_asset_dir

    def removed_from_edge_cache(self, file_name, expect_in_log=True):
        log_content = f"File /{file_name} is removed from CloudFront edge caches."
        if expect_in_log:
            self.assertIn(log_content, self.lmbr_aws_stdout)
        else:
            self.assertNotIn(log_content, self.lmbr_aws_stdout)
