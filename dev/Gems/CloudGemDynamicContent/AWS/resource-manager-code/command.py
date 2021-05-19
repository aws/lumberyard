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
# $Revision: #1 $

import types

import cloudfront
import content_manifest
import dynamic_content_migrator
import dynamic_content_settings
import resource_manager.cli
import signing
import staging
import dc_service_api

STAGING_STATUS_CHOICES = ['PUBLIC', 'PRIVATE', 'WINDOW']


def add_cli_commands(hook, subparsers, add_common_args, **kwargs):
    _default_manifest = dynamic_content_settings.get_default_manifest_name()

    subparser = subparsers.add_parser("dynamic-content", help="Commands to manage the CloudGemDynamicContent gem")
    subparser.register('action', 'parsers', resource_manager.cli.AliasedSubParsersAction)
    dynamic_content_subparsers = subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = dynamic_content_subparsers.add_parser('show-manifest', help='List all entries in the content manifest')
    subparser.add_argument('--manifest-path', required=False, help=f'Path to the manifest to use. If not provided assumes {_default_manifest}')
    subparser.add_argument('--manifest-version-id', required=False,
                           help='Version of the standalone manifest pak. Use the latest version if this argument is not specified')
    subparser.add_argument('--section', required=False, help='Section to show (Paks, Files)')
    subparser.add_argument('--file-name', required=False, help='File entry (local Folder + key) to show')
    subparser.add_argument('--platform-type', required=False, help='Platform of the file entry to list')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.list)

    subparser = dynamic_content_subparsers.add_parser('create-new-manifest', help='Create a new manifest')
    subparser.add_argument('--manifest-name', required=True, help='Name of the new manifest')
    subparser.add_argument('--manifest-path', required=False, help='Path to the new manifest to add')
    subparser.add_argument('--target-platforms', required=False, nargs='+', help='Target platforms for this new manifest (Default is all supported platforms)')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_new_manifest)

    subparser = dynamic_content_subparsers.add_parser('update-target-platforms', help='Update the target platform of a manifest')
    subparser.add_argument('--manifest-path', required=False, help='Path to the manifest to use')
    subparser.add_argument('--target-platforms', required=False, nargs='+',
                           help='Updated target platforms for this new manifest (Default is all supported platforms)')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.update_target_platforms)

    subparser = dynamic_content_subparsers.add_parser('add-manifest-file', aliases=['add-file'], help='Add a file to the content manifest for the project')
    subparser.add_argument('--file-name', required=True,
                           help='The name of the file including local folder e.g. staticdata/csv/gameproperties.csv where local folder becomes staticdata/csv/')
    subparser.add_argument('--cache-root', required=False, help='Local cache folder reference e.g.  @assets@')
    subparser.add_argument('--bucket-prefix', required=False, help='Bucket prefix to store in S3')
    subparser.add_argument('--manifest-path', required=False, help='Path to the manifest to use')
    subparser.add_argument('--output-root', required=False, help='Path to the default directory to write to')
    subparser.add_argument('--platform-type', required=False, help='Platform type this asset belongs to (Defaults to current platform)')
    subparser.add_argument('--section', required=False, help='Section to add to (Paks, Files)')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_add_file_entry)

    subparser = dynamic_content_subparsers.add_parser('remove-manifest-file', help='Remove a file from the content manifest for the project')
    subparser.add_argument('--file-name', required=True,
                           help='The name of the file including localfolder e.g. staticdata/csv/gameproperties.csv where local folder is staticdata/csv/ and key is gameproperties.csv')
    subparser.add_argument('--platform-type', required=True, help='The platform of the file to remove')
    subparser.add_argument('--manifest-path', required=False, help='Path to the manifest to use')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.remove_file_entry)

    subparser = dynamic_content_subparsers.add_parser('update-manifest', help='Update the manifest with current file hashes')
    subparser.add_argument('--manifest-path', required=False, help='Path to the manifest to use')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_update_file_hashes)

    subparser = dynamic_content_subparsers.add_parser('list-bucket-content', help='List a manifest\'s files found in the content bucket')
    subparser.add_argument('--manifest-path', required=False, help=f'Path to the manifest to use. If not provided assumes {_default_manifest} manifest')
    subparser.add_argument('--manifest-version-id', required=False,
                           help='Version of the standalone manifest pak. Use the latest version if this argument is not specified')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.list_bucket_content)

    subparser = dynamic_content_subparsers.add_parser('list-file-versions', help='List all versions of a manifest or pak file found in the content bucket.\
        Returns newest to oldest version based on uploaded date.')
    subparser.add_argument('--file-name', required=False, help='Name of the manifest or pak file')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_list_file_versions)

    subparser = dynamic_content_subparsers.add_parser('upload-manifest-content', help='Upload any changed manifest content to the content bucket')
    subparser.add_argument('--manifest-path', required=False, help='Path to the manifest to use')
    subparser.add_argument('--deployment-name', required=False, help='Which deployment to upload content to')
    subparser.add_argument('--staging-status', required=False, default='PRIVATE', choices=STAGING_STATUS_CHOICES,
                           help='What staging status should the content and manifest default to (eg PUBLIC/PRIVATE)')
    subparser.add_argument('--start-date', required=False,
                           help='Start date value for WINDOW staging (NOW or date/time in format January 15 2018 14:30) - UTC time')
    subparser.add_argument('--end-date', required=False,
                           help='End date value for WINDOW staging (NEVER or date/time in format January 15 2018 14:30) - UTC time')
    subparser.add_argument('--all', required=False, action='store_true',
                           help='Build and upload all paks. Ignores the results of file checks for content that has not changed')
    subparser.add_argument('--signing', required=False, action='store_true', help='Add file signatures to the content table for client side verification')
    subparser.add_argument('--invalidate-existing-files', required=False, action='store_true', help='Remove a file from CloudFront edge caches if exists')
    subparser.add_argument('--replace', required=False, action='store_true', help='Remove older versions when a new version has been uploaded')

    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_upload_manifest_content)

    subparser = dynamic_content_subparsers.add_parser('upload-folder', help='Upload all of the bundles found in a given folder')
    subparser.add_argument('--folder', required=True, help='Path to the folder upload bundles from')
    subparser.add_argument('--deployment-name', required=False, help='Which deployment to upload content to')
    subparser.add_argument('--staging-status', required=False, default='PRIVATE', choices=STAGING_STATUS_CHOICES,
                           help='What staging status should the content and manifest default to (eg PUBLIC/PRIVATE)')
    subparser.add_argument('--start-date', required=False,
                           help='Start date value for WINDOW staging (NOW or date/time in format January 15 2018 14:30) - UTC time')
    subparser.add_argument('--end-date', required=False,
                           help='End date value for WINDOW staging (NEVER or date/time in format January 15 2018 14:30) - UTC time')
    subparser.add_argument('--signing', required=False, action='store_true', help='Add file signatures to the content table for client side verification')
    subparser.add_argument('--bundle-type', required=False, default='pak', help='Type of bundle (by file extension, pak for example) to search for')
    subparser.add_argument('--invalidate-existing-files', required=False, action='store_true', help='Remove a file from CloudFront edge caches if exists')

    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.upload_folder_command)

    subparser = dynamic_content_subparsers.add_parser('upload-cf-key', help='Upload the cloudfront key to the dynamic content access bucket')
    subparser.add_argument('--key-path', required=True, help='Path to the private key file to upload')
    subparser.add_argument('--deployment-name', required=False, help='Which deployment to upload the key to')

    add_common_args(subparser)
    subparser.set_defaults(func=cloudfront.command_upload_cloudfront_key)

    subparser = dynamic_content_subparsers.add_parser('invalidate-file', help='Remove a file from CloudFront edge caches before it expires')
    subparser.add_argument('--file-path', required=True, help='File name in bucket')
    subparser.add_argument('--caller-reference', required=True, help='A value to uniquely identify an invalidation request.')

    add_common_args(subparser)
    subparser.set_defaults(func=cloudfront.command_invalidate_file)

    subparser = dynamic_content_subparsers.add_parser('compare-bucket-content', help='Compare manifest content to the bucket using HEAD Metadata checks')
    subparser.add_argument('--manifest-path', required=False, help=f'Path to the manifest to use. If missing, assumes {_default_manifest}')
    subparser.add_argument('--manifest-version-id', required=False, help='Version of the manifest. Use the latest version if this argument is not specified')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.compare_bucket_content)

    subparser = dynamic_content_subparsers.add_parser('clear-dynamic-content',
                                                      help='Empty the bucket and table content. Only delete the latest version if not specific argument is specified')
    subparser.add_argument('--all-versions', required=False, action='store_true', help='Remove all versions of dynamic content.')
    subparser.add_argument('--noncurrent-versions', required=False, action='store_true', help='Remove all the noncurrent versions of dynamic content.')
    subparser.add_argument('--confirm-deleting-noncurrent-versions', '-C', action='store_true',
                           help='Confirms that you know this command will delete all the noncurrent versions of files in the content bucket and ' \
                                'you will not be able to roll back to any previous version after this operation.')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_empty_content)

    subparser = dynamic_content_subparsers.add_parser('set-staging-status', help='Set the staging status of a given file')
    subparser.add_argument('--file-path', required=True, help='File name in bucket')
    subparser.add_argument('--version-id', required=False, help='Version id of the file')
    subparser.add_argument('--staging-status', required=True, choices=STAGING_STATUS_CHOICES,
                           help='Staging status (PUBLIC for available, WINDOW for available within start and end date)')
    subparser.add_argument('--start-date', required=False,
                           help='Start date value for WINDOW staging (NOW or date/time in format January 15 2018 14:30) - UTC time')
    subparser.add_argument('--end-date', required=False,
                           help='End date value for WINDOW staging (NEVER or date/time in format January 15 2018 14:30) - UTC time')
    subparser.add_argument('--include-children', required=False, action='store_true', help='Set the staging status for all the children paks')
    add_common_args(subparser)
    subparser.set_defaults(func=staging.command_set_staging_status)

    subparser = dynamic_content_subparsers.add_parser('request-url', help='Request a URL for the given file')
    subparser.add_argument('--file-path', required=True, help='File name in bucket')
    add_common_args(subparser)
    subparser.set_defaults(func=staging.command_request_url)

    subparser = dynamic_content_subparsers.add_parser('build-new-paks', help='Create pak files based on manifest files which have changed')
    subparser.add_argument('--manifest-path', required=False, help='Path to the manifest to use')
    subparser.add_argument('--all', required=False, action='store_true', help='Build all paks and update their manifest hash values regardless of file check')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_build_new_paks)

    subparser = dynamic_content_subparsers.add_parser('generate-keys', help='Generate a new public/private key pair for use by the Dynamic Content system')
    subparser.add_argument('--key-name', required=False, help='Key file name to use')
    add_common_args(subparser)
    subparser.set_defaults(func=signing.command_generate_keys)

    subparser = dynamic_content_subparsers.add_parser('show-signature', help='Show the signature which would be created for a given file')
    subparser.add_argument('--file-name', required=True, help='File name to sign')
    add_common_args(subparser)
    subparser.set_defaults(func=signing.command_show_signature)

    subparser = dynamic_content_subparsers.add_parser('test-signature', help='Tests whether a base64 signature is valid for a given string')
    subparser.add_argument('--signature', required=True, help='Base64 encoded signature')
    subparser.add_argument('--to-sign', required=True, help='String to sign')
    add_common_args(subparser)
    subparser.set_defaults(func=signing.test_signature)

    subparser = subparsers.add_parser('list-manifests', help='Lists all manifests for the current project include manifest dependencies.')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.list_manifests)

    subparser = dynamic_content_subparsers.add_parser('add-pak', help='Add a new pak entry to the manifest')
    subparser.add_argument('--pak-name', required=True, help='Name of the pak (final file name will be <pak-name>.<platform>.pak)')
    subparser.add_argument('--manifest-path', required=True, help='Path to the manifest to use')
    subparser.add_argument('--platform-type', required=True, help='Pak to add the file to')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_add_pak)

    subparser = dynamic_content_subparsers.add_parser('add-file-to-pak', help='Add a given file to the specified pak')
    subparser.add_argument('--file-name', required=True, help='File entry to add')
    subparser.add_argument('--platform-type', required=True, help='Platform of the file entry to add')
    subparser.add_argument('--manifest-path', required=True, help='Path to the manifest to use')
    subparser.add_argument('--pak-file', required=True, help='Pak to add the file to')
    add_common_args(subparser)
    subparser.set_defaults(func=content_manifest.command_add_file_to_pak)

    subparser = dynamic_content_subparsers.add_parser('migrate-staging-settings',
                                                      help='Migrate existing staging settings information when content versioning is enabled or suspended')
    subparser.add_argument('--deployment-name', required=False, help='Which deployment to migrate existing staging settings information for')
    add_common_args(subparser)
    subparser.set_defaults(func=dynamic_content_migrator.command_migrate_table_entries)

    subparser = dynamic_content_subparsers.add_parser('suspend-versioning', help='Suspend dynamic content versioning')
    subparser.add_argument('--deployment-name', required=False, help='Which deployment to suspend versioning for')
    subparser.add_argument('--confirm-versioning-suspension', '-C', action='store_true',
                           help='Confirms that you know this command will suspend content versioning and ' \
                                'you need to update the deployment afterwards.')
    add_common_args(subparser)
    subparser.set_defaults(func=dynamic_content_migrator.command_suspend_versioning)

    subparser = dynamic_content_subparsers.add_parser('list-uploaded-content', help='List all the uploaded content')
    add_common_args(subparser)
    subparser.set_defaults(func=dc_service_api.command_show_uploaded)

    subparser = dynamic_content_subparsers.add_parser('delete-uploaded-content', help='Delete an uploaded manifest file or PAK')
    subparser.add_argument('--file-path', required=True, help='File name in bucket')
    subparser.add_argument('--version-id', required=False, help='Version id of the file')
    subparser.add_argument('--confirm-deletion', '-C', action='store_true',
                           help='Confirms that file and its children (if any) will be deleted')
    add_common_args(subparser)
    subparser.set_defaults(func=dc_service_api.command_delete_uploaded)

    subparser = dynamic_content_subparsers.add_parser('show-logs', help='Show recent log events for ServiceLambda')
    subparser.add_argument('--minutes', type=int, required=False, help='How far back from now to attempt to display. Default is 10mins')
    add_common_args(subparser)
    subparser.set_defaults(func=dc_service_api.command_show_log_events)


def add_cli_view_commands(hook, view_context, **kwargs):
    def list_manifests(self, manifests):
        for manifest in manifests:
            self._output_message(manifest)

    view_context.list_manifests = types.MethodType(list_manifests, view_context)

    def create_new_manifest(self, manifest):
        self._output_message('New manifest {} is created'.format(manifest))

    view_context.create_new_manifest = types.MethodType(create_new_manifest, view_context)

    def update_target_platforms(self, manifest):
        self._output_message('Target platforms for the manifest {} are updated'.format(manifest))

    view_context.update_target_platforms = types.MethodType(update_target_platforms, view_context)

    def show_manifest_file(self, filesList):
        for manifest in filesList:
            key_name = manifest.get('keyName', '')
            hash = manifest.get('hash', '')
            content_hash = manifest.get('contentHash', '')
            version_id = manifest.get('versionId')
            cache_root = manifest.get('cacheRoot', '')
            local_folder = manifest.get('localFolder', '')
            bucket_prefix = manifest.get('BucketPrefix', '')
            output_dir = manifest.get('outputDir', '')
            pak_file = manifest.get('pakFile', '')
            platform_type = manifest.get('platformType', '')
            if version_id:
                message = f'\nFile Key: {key_name}\nHash: {hash}\nContent Hash: {content_hash}\nVersion ID: {version_id}\nCache Root: {cache_root}\nLocal Folder: {local_folder}\nBucket Prefix: {bucket_prefix}\noutputDir: {output_dir}\npakFile: {pak_file}\nplatformType: {platform_type}'
            else:
                message = f'\nFile Key: {key_name}\nHash: {hash}\nContent Hash: {content_hash}\nCache Root: {cache_root}\nLocal Folder: {local_folder}\nBucket Prefix: {bucket_prefix}\noutputDir: {output_dir}\npakFile: {pak_file}\nplatformType: {platform_type}'
            self._output_message(message)

    view_context.show_manifest_file = types.MethodType(show_manifest_file, view_context)

    def show_bucket_diff(self, bucketDiffData):
        for key, value in bucketDiffData.items():
            self._output_message(key)
            for file in value:
                self._output_message('\t' + file)

    view_context.show_bucket_diff = types.MethodType(show_bucket_diff, view_context)

    def finding_updated_content(self, manifestPath):
        self._output_message('Searching for updated content from {}'.format(manifestPath))

    view_context.finding_updated_content = types.MethodType(finding_updated_content, view_context)

    def invalid_file(self, filePath):
        self._output_message('Invalid File: {}'.format(filePath))

    view_context.invalid_file = types.MethodType(invalid_file, view_context)

    def hash_comparison_disk(self, filePath, manifestHash, diskHash):
        self._output_message('Comparing hashes for {}'.format(filePath))
        if manifestHash == diskHash:
            self._output_message(' MATCH Manifest: {}  Disk: {}'.format(manifestHash, diskHash))
        else:
            self._output_message(' UPDATE Manifest: {}  Disk: {}'.format(manifestHash, diskHash))

    view_context.hash_comparison_disk = types.MethodType(hash_comparison_disk, view_context)

    def no_stack(self):
        pass

    view_context.no_stack = types.MethodType(no_stack, view_context)

    def update_percent_complete(self, percentage):
        self._output_message("%2.2f\n" % percentage)

    view_context.update_percent_complete = types.MethodType(update_percent_complete, view_context)

    def found_updated_item(self, keyName):
        pass

    view_context.found_updated_item = types.MethodType(found_updated_item, view_context)

    def done_uploading(self, filePath):
        pass

    view_context.done_uploading = types.MethodType(done_uploading, view_context)


def add_gui_commands(hook, handlers, **kwargs):
    handlers.update({
        'list-manifests': content_manifest.gui_list_manifests,
        'show-manifest': content_manifest.gui_list,
        'new-manifest': content_manifest.gui_new_manifest,
        'delete-manifest': content_manifest.gui_delete_manifest,
        'add-files-to-manifest': content_manifest.gui_add_files_to_manifest,
        'delete-files-from-manifest': content_manifest.gui_delete_files_from_manifest,
        'add-pak-to-manifest': content_manifest.gui_add_pak_to_manifest,
        'delete-pak-from-manifest': content_manifest.gui_delete_pak_from_manifest,
        'add-files-to-pak': content_manifest.gui_add_files_to_pak,
        'delete-files-from-pak': content_manifest.gui_delete_files_from_pak,
        'pak-and-upload': content_manifest.gui_pak_and_upload,
        'get-bucket-status': content_manifest.gui_get_bucket_status,
        'get-local-file-status': content_manifest.gui_get_local_file_status,
        'generate-keys': content_manifest.gui_generate_keys,
        'change-target-platforms': content_manifest.gui_change_target_platforms,
        'get-full-platform-cache-game-path': content_manifest.gui_get_full_platform_cache_game_path,
        'check-existing-keys': content_manifest.gui_check_existing_keys
    })


def add_gui_view_commands(hook, view_context, **kwargs):
    def list_manifests(self, manifests):
        self._output('list-manifests', manifests)

    view_context.list_manifests = types.MethodType(list_manifests, view_context)

    def show_manifest_file(self, sections):
        self._output('show-manifest', sections)

    view_context.show_manifest_file = types.MethodType(show_manifest_file, view_context)

    def show_bucket_diff(self, bucketDiffData):
        self._output('get-bucket-status', bucketDiffData)

    view_context.show_bucket_diff = types.MethodType(show_bucket_diff, view_context)

    def show_full_platform_cache_game_path(self, platform_cache_root):
        self._output('get-full-platform-cache-game-path', platform_cache_root)

    view_context.show_full_platform_cache_game_path = types.MethodType(show_full_platform_cache_game_path, view_context)

    def show_local_file_diff(self, localFileDiffList):
        self._output('get-local-file-status', localFileDiffList)

    view_context.show_local_file_diff = types.MethodType(show_local_file_diff, view_context)

    def gui_signal_upload_complete(self):
        self._output('signal-upload-complete', 'ok')

    view_context.gui_signal_upload_complete = types.MethodType(gui_signal_upload_complete, view_context)

    def finding_updated_content(self, manifestPath):
        pass  # these are used in a function which we want output to cli but not gui

    view_context.finding_updated_content = types.MethodType(finding_updated_content, view_context)

    def invalid_file(self, manifestPath):
        pass  # these are used in a function which we want output to cli but not gui

    view_context.invalid_file = types.MethodType(invalid_file, view_context)

    def hash_comparison_disk(self, filePath, manifestHash, diskHash):
        pass  # these are used in a function which we want output to cli but not gui

    view_context.hash_comparison_disk = types.MethodType(hash_comparison_disk, view_context)

    def no_stack(self):
        self._output('no-stack', 'ok')

    view_context.no_stack = types.MethodType(no_stack, view_context)

    def update_percent_complete(self, percentage):
        self._output('percent-complete', str(int(percentage)) + '%')

    view_context.update_percent_complete = types.MethodType(update_percent_complete, view_context)

    def found_updated_item(self, keyName):
        self._output('found-update-item', {'message': 'Found updated item {}'.format(keyName), 'keyName': keyName})

    view_context.found_updated_item = types.MethodType(found_updated_item, view_context)

    def done_uploading(self, filePath):
        self._output('done-uploading', {'message': 'Done uploading {}'.format(filePath), 'filePath': filePath})

    view_context.done_uploading = types.MethodType(done_uploading, view_context)

    def check_existing_keys(self, key_exists):
        self._output('check-existing-keys', key_exists)

    view_context.check_existing_keys = types.MethodType(check_existing_keys, view_context)

    def generate_keys_completed(self):
        self._output('generate-keys', 'Keys were generated successfully.')

    view_context.generate_keys_completed = types.MethodType(generate_keys_completed, view_context)
