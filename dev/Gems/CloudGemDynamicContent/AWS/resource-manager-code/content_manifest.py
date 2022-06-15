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
import glob
import hashlib
import os
import posixpath
import re
import threading
from functools import wraps
from io import BytesIO
from sys import platform as _platform
from typing import Tuple

from boto3.s3.transfer import S3Transfer
from boto3.s3.transfer import TransferConfig
from botocore.exceptions import ClientError

import cloudfront
import content_bucket
import dynamic_content_settings
import pak_files
import show_manifest
import signing
import staging
from path_utils import ensure_posix_path
from resource_manager.errors import HandledError
from six import iteritems  # Python 2.7/3.7 Compatibility


def list_manifests(context, args):
    # this will need to look for manifest references and build a tree
    # new code should give the folder directly, so remove the path.dirname when possible
    manifest_path = os.path.normpath(os.path.dirname(_get_default_manifest_path(context)))
    manifests = glob.glob(os.path.join(manifest_path, '*' + os.path.extsep + 'json'))
    context.view.list_manifests(manifests)


def list(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path, args.manifest_version_id)
    files_list = _get_files_list(context, manifest, args.section)
    if args.file_name:
        files_list = [thisEntry for thisEntry in files_list if entry_matches_file(thisEntry, args.file_name, args.platform_type)]

    context.view.show_manifest_file(files_list)
    _clear_versioned_manifest(manifest_path, args.manifest_version_id)


def _clear_versioned_manifest(manifest_path, manifest_version_id=None):
    """
        Remove the downloaded versioned manifest file

        Arguments
            manifest_path -- path to the manifest
            manifest_version_id -- version ID of the manifest
    """
    if not manifest_version_id:
        return

    manifest_path += f'.{manifest_version_id}'
    if os.path.exists(manifest_path):
        os.remove(manifest_path)


def gui_is_stack_configured(context):
    try:
        stack_id = context.config.get_resource_group_stack_id(context.config.default_deployment,
                                                              dynamic_content_settings.get_default_resource_group(),
                                                              optional=True)
    except:
        stack_id = None

    return stack_id is not None


def stack_required(function):
    @wraps(function)
    def wrapper(*args, **kwargs):
        context = args[0]
        if not gui_is_stack_configured(context):
            context.view.no_stack()
            return
        return function(*args, **kwargs)

    return wrapper


@stack_required
def gui_list(context, args, pak_status=None):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    sections = {
        'Files': _get_files_list(context, manifest, 'Files'),
        'Paks': _get_files_list(context, manifest, 'Paks'),
        'Platforms': _get_platforms_list(context, manifest),
        'PakStatus': pak_status
    }
    context.view.show_manifest_file(sections)


def validate_manifest_name(manifest_name):
    regex_str = '^[-0-9a-zA-Z!_][-0-9a-zA-Z!_.]*$'
    return re.match(regex_str, manifest_name) is not None


@stack_required
def gui_list_manifests(context, args):
    list_manifests(context, args)


@stack_required
def gui_new_manifest(context, args):
    if not validate_manifest_name(args.new_file_name):
        raise HandledError('Invalid manifest name')
    manifest_path = os.path.normpath(os.path.dirname(_get_default_manifest_path(context)))
    new_file_name = os.path.join(manifest_path, args.new_file_name + os.path.extsep + 'json')
    new_file_platforms = args.platform_list
    manifests = glob.glob(os.path.join(manifest_path, '*' + os.path.extsep + 'json'))
    if new_file_name in manifests:
        raise HandledError('File already exists')
    new_manifest(context, new_file_name, new_file_platforms)
    manifests.append(new_file_name)
    context.view.list_manifests(manifests)


@stack_required
def gui_delete_manifest(context, args):
    delete_manifest(context, args.manifest_path)
    list_manifests(context, args)


@stack_required
def gui_change_target_platforms(context, args):
    change_target_platforms(context, args.platform_list, args.manifest_path)


def change_target_platforms(context, target_platforms, manifest_path):
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    if manifest.get('Metadata', {}):
        manifest['Metadata']['Platforms'] = target_platforms
    else:
        manifest['Metadata'] = {'Platforms': target_platforms}
    _save_content_manifest(context, manifest_path, manifest)


@stack_required
def gui_add_files_to_manifest(context, args):
    files_to_add = args.file_list
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    for file_info in files_to_add:
        file_name = file_info.get('fileName')
        platformType = file_info.get('platformType')
        add_file_entry(context, manifest_path, file_name, 'Files', platform_type=platformType)
    gui_list(context, args)


@stack_required
def gui_delete_files_from_manifest(context, args):
    files_to_remove = args.file_list
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    for file_info in files_to_remove:
        file_name = file_info.get('fileName')
        platform = file_info.get('platformType')
        _remove_file_entry_from_section(context, file_name, platform, manifest_path, manifest, args.section)
    gui_list(context, args)


@stack_required
def gui_add_pak_to_manifest(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    new_file_name = args.new_file_name + os.path.extsep + 'pak'
    platform_type = args.platform_type
    _add_pak_to_manifest(context, os.path.join(dynamic_content_settings.get_pak_folder(), new_file_name), manifest_path, manifest, platform_type)
    gui_list(context, args, {new_file_name: 'new'})


@stack_required
def gui_delete_pak_from_manifest(context, args):
    file_info = args.file_data
    file_name = file_info.get('keyName')
    platform = file_info.get('platformType')

    file_to_remove = os.path.join(dynamic_content_settings.get_pak_folder(), file_name)
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    _remove_file_entry_from_section(context, file_to_remove, platform, manifest_path, manifest, 'Paks')
    existing_file_list = _get_files_list(context, manifest, 'Files')
    for existing_file in existing_file_list:
        if 'pakFile' in existing_file:
            if existing_file['pakFile'] == file_to_remove:
                del existing_file['pakFile']
    _save_content_manifest(context, manifest_path, manifest)
    gui_list(context, args)


@stack_required
def gui_add_files_to_pak(context, args):
    files_to_add = args.file_list
    pak_file = os.path.join(dynamic_content_settings.get_pak_folder(), args.pak_file)
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)

    for new_file in files_to_add:
        file_name = ensure_posix_path(os.path.join(new_file.get('localFolder'), new_file.get('keyName')))
        file_platform = new_file.get('platformType')
        _add_file_to_pak(context, file_name, file_platform, pak_file, manifest_path, manifest)

    gui_list(context, args, {os.path.basename(pak_file): 'outdated'})


@stack_required
def gui_delete_files_from_pak(context, args):
    file_to_remove = args.file_data
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    file_list = _get_files_list(context, manifest, 'Files')

    pak_status = {}
    for existing_file in file_list:
        if _compare_path(os.path.join(existing_file['localFolder'], existing_file['keyName']), file_to_remove['keyName']) and existing_file['platformType'] == \
                file_to_remove['platformType']:
            pak_status[os.path.basename(existing_file['pakFile'])] = 'outdated'
            del existing_file['pakFile']
    manifest['Files'] = file_list
    _save_content_manifest(context, manifest_path, manifest)
    gui_list(context, args, pak_status)


def gui_pak_and_upload(context, args):
    if not gui_is_stack_configured(context):
        context.view.gui_signal_upload_complete()
        return
    if not args.staging_status:
        args.staging_status = "PRIVATE"
    command_upload_manifest_content(context, args)
    pak_status = {}
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    pak_list = _get_files_list(context, manifest, 'Paks')
    for pak in pak_list:
        pak_status[pak['keyName']] = 'match'
    gui_list(context, args, pak_status)
    context.view.show_local_file_diff([])  # all files up to date, so clear status on all of them
    context.view.gui_signal_upload_complete()


@stack_required
def gui_get_bucket_status(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    contents_list = content_bucket.get_bucket_content_list(context)
    manifest_dict = _create_manifest_bucket_key_map(context, manifest, manifest_path)
    bucket_diff_data = {'new': [], 'outdated': [], 'match': []}
    for this_bucket_item in contents_list:
        this_key = this_bucket_item.get('Key')
        if this_key in manifest_dict:
            matched, version_id = synchronize_manifest_entry_with_bucket(context, this_key, manifest_dict[this_key])
            if matched:
                bucket_diff_data['match'].append(this_key)
            else:
                bucket_diff_data['outdated'].append(this_key)
            del manifest_dict[this_key]
    for remaining_key in manifest_dict.keys():
        bucket_diff_data['new'].append(remaining_key)
    context.view.show_bucket_diff(bucket_diff_data)


@stack_required
def gui_get_full_platform_cache_game_path(context, args):
    full_platform_cache_game_path = _get_full_platform_cache_game_path(context, args.platform, context.config.game_directory_name)
    context.view.show_full_platform_cache_game_path({"type": args.type, "path": full_platform_cache_game_path, 'platform': args.platform})


@stack_required
def gui_check_existing_keys(context, args):
    key_exists = False
    public_key_file, private_key_file = signing.get_key_pair_paths(context, None)
    if os.path.exists(public_key_file):
        key_exists = True
    context.view.check_existing_keys({'keyExists': key_exists, 'publicKeyFile': public_key_file})


@stack_required
def gui_generate_keys(context, args):
    signing.command_generate_keys(context, args)
    context.view.generate_keys_completed()


@stack_required
def gui_get_local_file_status(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    updated_content = _get_updated_local_content(context, manifest_path, manifest, False)
    local_status = []
    for platform, files in iteritems(updated_content):
        for fileEntry in files:
            local_status.append(posixpath.join(fileEntry['localFolder'], fileEntry['keyName']))
    context.view.show_local_file_diff(local_status)


def _compare_path(a, b):
    return os.path.normcase(os.path.normpath(a)) == os.path.normcase(os.path.normpath(b))


def new_manifest(context, manifest_path, manifest_platforms):
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    manifest['Files'] = []
    manifest['Metadata'] = {'Platforms': manifest_platforms}
    _save_content_manifest(context, manifest_path, manifest)


def delete_manifest(context, manifest_path):
    os.remove(manifest_path)  # TODO: offer to delete associated paks?


def is_manifest_entry(thisEntry):
    local_folder = thisEntry.get('localFolder')
    local_folder = make_end_in_slash(local_folder)

    return local_folder.endswith(dynamic_content_settings.get_manifest_folder())


def validate_add_key_name(file_name):
    if len(file_name) >= 100:  # AWS Limit
        raise HandledError('File name too long')
    regex_str = '^[-0-9a-zA-Z!_\.\(\)/\\\]*$'
    if not re.match(regex_str, file_name):
        raise HandledError('File does not match naming rules')


def command_new_manifest(context, args):
    if not validate_manifest_name(args.manifest_name):
        raise HandledError('Invalid manifest name')

    manifest_path = determine_manifest_path(context, args.manifest_path)
    manifest_dir_path = os.path.normpath(manifest_path)
    new_manifest_name = os.path.join(manifest_dir_path, args.manifest_name + os.path.extsep + 'json')
    manifests = glob.glob(os.path.join(manifest_dir_path, '*' + os.path.extsep + 'json'))
    if new_manifest_name in manifests:
        raise HandledError('Manifest already exists')
    new_manifest(context, new_manifest_name, args.target_platforms)
    manifests.append(new_manifest_name)

    context.view.create_new_manifest(args.manifest_name)


def update_target_platforms(context, args):
    manifest_path = determine_manifest_path(context, args.manifest_path)
    manifest_dir_path = os.path.normpath(os.path.dirname(manifest_path))
    manifests = glob.glob(os.path.join(manifest_dir_path, '*' + os.path.extsep + 'json'))
    if os.path.normpath(manifest_path) not in manifests:
        raise HandledError('Manifest does not exist')

    target_platforms = args.target_platforms
    if target_platforms is None:
        target_platforms = []
    change_target_platforms(context, target_platforms, manifest_path)

    context.view.update_target_platforms(manifest_path)


def command_add_pak(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    platform_type = args.platform_type
    new_file_name = args.pak_name + os.path.extsep + platform_type + os.path.extsep + 'pak'
    _add_pak_to_manifest(context, os.path.join(dynamic_content_settings.get_pak_folder(), new_file_name),
                         manifest_path, manifest, platform_type)


def entry_matches_platform(this_entry, platform_type):
    this_platform = this_entry.get('platformType')

    if this_platform and this_platform != 'shared' and platform_type != 'shared' and platform_type != this_platform:
        print('Platforms do not match {} vs {}'.format(this_platform, platform_type))
        return False
    return True


def entry_matches_file(this_entry, file_name, file_platform):
    local_folder = this_entry.get('localFolder')
    key_name = this_entry.get('keyName')
    platform_type = this_entry.get('platformType')

    if local_folder == '.' and key_name == file_name and platform_type == file_platform:
        return True

    existing_file_name = ensure_posix_path(os.path.join(local_folder, key_name))
    print('Comparing {} vs {}'.format(existing_file_name, file_name))
    return existing_file_name == file_name and platform_type == file_platform


def command_add_file_to_pak(context, args):
    pak_file = os.path.join(dynamic_content_settings.get_pak_folder(), args.pak_file)
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    file_name = args.file_name
    file_platform = args.platform_type
    _add_file_to_pak(context, file_name, file_platform, pak_file, manifest_path, manifest)


def _add_file_to_pak(context, file_name, file_platform, pak_file, manifest_path, manifest):
    pak_list = _get_paks_list(context, manifest)
    pak_platform_type = ''
    pak_found = False
    pak_entry = {}
    for pak_entry in pak_list:
        if pak_entry['pakFile'] == pak_file:
            pak_platform_type = pak_entry['platformType']
            pak_found = True
            break

    if not pak_found:
        raise HandledError('No pak named {}'.format(pak_file))

    file_list = _get_files_list(context, manifest, 'Files')
    file_found = False
    for file_entry in file_list:
        if not entry_matches_platform(file_entry, pak_platform_type):
            continue

        if entry_matches_file(file_entry, file_name, file_platform):
            file_entry['pakFile'] = pak_file
            file_entry['hash'] = ''  # Need to be sure to add this to the pak next update
            platform_name = file_entry.get('platformType')
            file_found = True
            break
    if not file_found:
        raise HandledError('No matching file found {} platform {}'.format(file_name, pak_platform_type))
    manifest['Files'] = file_list
    _save_content_manifest(context, manifest_path, manifest)


def command_add_file_entry(context, args):
    add_file_entry(context, args.manifest_path, args.file_name, args.file_section, None, args.cache_root, args.bucket_prefix, args.output_root,
                   args.platform_type)


def add_file_entry(context, manifest_path, file_path, manifest_section, pakFileEntry=None, cache_root=None,
                   bucket_prefix=None, output_root=None, platform_type=None):
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    file_section = manifest_section
    if file_section is None:
        file_section = 'Files'
    this_file = {}
    if file_path is None:
        raise HandledError('No file name specified')

    file_path = ensure_posix_path(file_path)

    name_pair = os.path.split(file_path)
    file_name = name_pair[1]
    local_path = name_pair[0]
    if len(local_path) and local_path[0] == '/':
        local_path = local_path[1:]
    if file_name is None:
        raise HandledError('No file name specified')

    validate_add_key_name(file_name)
    this_file['keyName'] = file_name
    if not (local_path and len(local_path)):
        local_path = '.'

    this_file['cacheRoot'] = cache_root or '@assets@'
    this_file['localFolder'] = local_path
    this_file['bucketPrefix'] = bucket_prefix or ''
    this_file['outputRoot'] = output_root or '@user@'
    this_file['platformType'] = platform_type or ''

    if pakFileEntry is not None:
        this_file['pakFile'] = pakFileEntry or ''
    this_file['isManifest'] = is_manifest_entry(this_file)

    existing_list = _get_files_list(context, manifest, file_section)
    files_list = [thisEntry for thisEntry in existing_list if not (thisEntry.get('keyName') == file_name
                                                                   and thisEntry.get('localFolder', '.') == local_path
                                                                   and thisEntry.get('platformType') == platform_type)]

    files_list.append(this_file)
    manifest[file_section] = files_list
    _save_content_manifest(context, manifest_path, manifest)


def _get_relative_path_after_game_dir(context, pak_path):
    replace_path = ensure_posix_path(pak_path)
    try:
        return replace_path.split(context.config.game_directory_name + '/')[1]
    except:
        return replace_path


def _add_pak_to_manifest(context, pak_path, manifest_path, manifest, platform_name):
    paks_list = _get_paks_list(context, manifest)
    potential_pak_file_entry_path = _get_relative_path_after_game_dir(context, pak_path)
    pak_already_listed_in_manifest = False
    for pakEntryToCheck in paks_list:
        if pakEntryToCheck.get('pakFile') == potential_pak_file_entry_path:
            pak_already_listed_in_manifest = True
            break
    if not pak_already_listed_in_manifest:
        add_file_entry(context, manifest_path, potential_pak_file_entry_path, 'Paks', potential_pak_file_entry_path, './',
                       platform_type=platform_name)


def _remove_file_entry_from_section(context, file_name, platform, manifest_path, manifest, section):
    if section is None:
        raise HandledError('No section specified to remove file entry from')
    if file_name is None:
        raise HandledError('No file name specified')

    files_list = _get_files_list(context, manifest, section)

    name_pair = os.path.split(file_name)
    file_name = name_pair[1]
    local_path = name_pair[0]
    found_entry = False
    for thisFile in files_list:
        if thisFile['keyName'] == file_name and thisFile['localFolder'] == local_path \
                and thisFile['platformType'] == platform:
            files_list.remove(thisFile)
            found_entry = True
    manifest[section] = files_list
    if found_entry:
        _save_content_manifest(context, manifest_path, manifest)
    return found_entry


def validate_writable(context, manifest_path):
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    if not context.config.validate_writable(manifest_path):
        show_manifest.manifest_not_writable(manifest_path)
        return False
    return True


def remove_file_entry(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    if args.file_name is None:
        raise HandledError('No file name specified')
    if args.platform_type is None:
        raise HandledError('No platform type specified')

    found_entry = _remove_file_entry_from_section(context, args.file_name, args.platform_type, manifest_path, manifest, 'Files')
    if not found_entry:
        found_entry = _remove_file_entry_from_section(context, args.file_name, args.platform_type, manifest_path, manifest, 'Paks')
    if not found_entry:
        raise HandledError('Key not found {} with platform type {}'.format(args.file_name, args.platform_type))


def _get_files_list(context, manifest, section=None):
    if manifest is None:
        raise HandledError('No manifest data loaded')

    if section is not None:
        files_list = manifest.get(section)

        if files_list is None:
            if section not in ['Files', 'Paks']:
                raise HandledError('No section {} found in manifest'.format(section))
            files_list = []

    else:
        files_list = []
        files_list.extend(manifest.get('Files', []))
        files_list.extend(manifest.get('Paks', []))

    return files_list


def _get_platforms_list(context, manifest):
    if manifest is None:
        raise HandledError('No manifest data loaded')

    return manifest.get('Metadata', {}).get('Platforms', [])


def _get_paks_list(context, manifest):
    if manifest is None:
        raise HandledError('No manifest data loaded')
    paks_list = manifest.get('Paks', [])
    if paks_list is None:
        raise HandledError('No Paks list found in manifest')
    return paks_list


def _get_default_manifest_path(context):
    base_path = dynamic_content_settings.get_manifest_game_folder(context.config.game_directory_path)
    return_path = os.path.join(base_path, dynamic_content_settings.get_default_manifest_name())
    return return_path


def determine_manifest_path(context, provided_name):
    if provided_name is None:
        manifest_path = _get_default_manifest_path(context)
    else:
        manifest_path = os.path.join(
            dynamic_content_settings.get_manifest_game_folder(context.config.game_directory_path), provided_name)
    return manifest_path


def _get_path_and_manifest(context: object, provided_name: str, version_id: str = '') -> Tuple[str, dict]:
    """
        Get the path and content of the manifest on disk.

        Arguments
            context -- context to use
            provided_name -- name of the manifest
            version_id -- version ID of the manifest. Manifest will be downloaded from S3 first if specified
    """
    versioned_manifest_path = manifest_path = determine_manifest_path(context, provided_name)
    if version_id:
        # Download a specific version of standalone manifest pak from S3
        versioned_manifest_path = manifest_path + f'.{version_id}'
        bucket_name = content_bucket.get_content_bucket(context)
        s3 = context.aws.resource('s3')
        try:
            manifest_pak_obj = BytesIO()
            s3.meta.client.download_fileobj(
                Bucket=bucket_name,
                Key=get_standalone_manifest_pak_key(context, manifest_path),
                Fileobj=manifest_pak_obj,
                ExtraArgs={'VersionId': version_id})
            manifest_pak_obj.seek(0)

            manifest_path_in_pak = os.path.join(dynamic_content_settings.get_manifest_folder(), provided_name)
            manifest_path_in_pak = ensure_posix_path(os.path.normcase(manifest_path_in_pak))
            pak_files.extract_from_pak_object(manifest_pak_obj, manifest_path_in_pak, versioned_manifest_path)
        except Exception as e:
            print(f'Failed to download the manifest version {version_id}. Error: {str(e)}')

    if os.path.exists(versioned_manifest_path):
        manifest = context.config.load_json(versioned_manifest_path)
    else:
        print(f'Invalid manifest path {versioned_manifest_path}')
        manifest = {}
    return manifest_path, manifest


def command_update_file_hashes(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    _update_file_hashes(context, manifest_path, manifest)


def _update_file_hash_section(context, manifest_path, manifest, section):
    if section is None:
        raise HandledError('No specified to update hashes for')

    files_list = _get_files_list(context, manifest, section)
    content_hash_dict = _create_content_hash_dict(context, manifest, section)

    show_manifest.updating_file_hashes(manifest_path)
    files_updated = False
    for this_file in files_list:
        this_file_path = _get_path_for_file_entry(context, this_file, context.config.game_directory_name)
        if not os.path.isfile(this_file_path):
            show_manifest.invalid_file(this_file_path)
            this_file['hash'] = ''
            this_file['size'] = None
            continue

        hex_return = hashlib.md5(open(this_file_path, 'rb').read()).hexdigest()
        manifest_hash = this_file.get('hash', '')
        if hex_return != manifest_hash:
            files_updated = True

        if section == 'Files':
            show_manifest.hash_comparison_disk(this_file_path, manifest_hash, hex_return)
        else:
            # Update and compare the content hash of pak files
            content_hash = content_hash_dict.get(this_file.get('pakFile', ''), {})
            hex_content_hash = content_hash.hexdigest() if content_hash else ''
            manifest_content_hash = this_file.get('contentHash', '')
            if hex_content_hash != manifest_content_hash:
                files_updated = True
            show_manifest.content_hash_comparison_disk(this_file_path, manifest_content_hash, hex_content_hash)
            this_file['contentHash'] = hex_content_hash

        this_file['hash'] = hex_return
        file_stat = os.stat(this_file_path)
        this_file['size'] = file_stat.st_size
    manifest[section] = files_list
    return files_updated


def _create_content_hash_dict(context: object, manifest: dict, section: str) -> dict:
    """ Calculate the content hash dictionary. Pak files with same archive members can get 
        different standard hash values (md5 hash of the pak file itself) because of the metadata
        like modified time and order of archive members. However, they should always get the 
        same content hash (the hash of all the archive member hash).
    
    Arguments
        context -- context to use
        manifest -- manifest dictionary loaded from the manifest file
        section -- section of the manifest files
    """
    if section == 'Files':
        # Only pak files have content hash
        return {}

    # Iterate through each file in the manifest and re-calculate the content hash of the pak it belongs to
    files_list = _get_files_list(context, manifest, 'Files')
    content_hash_dict = {}
    for this_file in files_list:
        pak_file = this_file.get('pakFile', '')
        if not pak_file:
            # The current file isn't in any pak
            continue
        if pak_file not in content_hash_dict:
            content_hash_dict[pak_file] = hashlib.md5()

        content_hash_dict[pak_file].update(this_file['hash'].encode())
    return content_hash_dict


def _update_file_hashes(context, manifest_path, manifest):
    need_save = _update_file_hash_section(context, manifest_path, manifest, 'Files')
    need_save |= _update_file_hash_section(context, manifest_path, manifest, 'Paks')
    if need_save:
        _save_content_manifest(context, manifest_path, manifest)


def _save_content_manifest(context, filePath, manifestData):
    context.config.validate_writable(filePath)
    context.config.save_json(filePath, manifestData)
    print("Updated manifest")


def _head_bucket_current_item(context: object, file_key: str, deployment_name: str = '') -> Tuple['hash_metadata', str]:
    """ Get the hash metadata and version id of the bucket item 
    
    Arguments
        s3_client -- boto3 client for s3
        bucket_name -- name of the s3 bucket
        file_key -- key of the bucket item
    """
    bucket_name = content_bucket.get_content_bucket_by_name(context, deployment_name)
    s3 = context.aws.client('s3')
    head_response = {}
    try:
        head_response = s3.head_object(
            Bucket=bucket_name,
            Key=file_key
        )
    except ClientError as e:
        if e.response['Error']['Message'] == 'Not Found':
            show_manifest.key_not_found(file_key)
        else:
            HandledError(f'Could not get the head response for s3 object {file_key}', e)

    return _get_hash_metadata_from_head_response(head_response), _get_version_id_from_head_response(head_response)


def _update_bucket_hash_metadata(context: object, file_key: str, bucket_hash_metadata: 'hash_metadata', deployment_name='') -> None:
    """ Update the hash and content hash metadata of the bucket item
    
    Arguments
        s3_client -- s3 client to use
        bucket_name -- name of the s3 bucket
        file_key -- key of the bucket item
        bucket_hash_metadata -- hash metadata of the bucket item
    """
    bucket_name = content_bucket.get_content_bucket_by_name(context, deployment_name)
    s3 = context.aws.client('s3')
    try:
        # S3 doesn't allow modification of the object metadata. Instead, we need to copy the object with any updated metadata
        s3.copy_object(
            Bucket=bucket_name,
            Key=file_key,
            CopySource={
                'Bucket': bucket_name,
                'Key': file_key
            },
            Metadata={
                _get_meta_hash_name(): bucket_hash_metadata.hash,
                _get_meta_content_hash_name(): bucket_hash_metadata.hash
            },
            MetadataDirective='REPLACE')
    except Exception as e:
        raise HandledError(f'Could not update the hash metadata for s3 object {file_key}', e)


def command_empty_content(context, args):
    if args.all_versions and args.noncurrent_versions:
        print('Argument all_versions and noncurrent_versions should not be specified at the same time')
        return
    if args.noncurrent_versions:
        message = 'Deleting noncurrent versions is nonreversible and you will not be able to roll back to any previous version after this operation.'
        if not args.confirm_deleting_noncurrent_versions:
            context.view.confirm_message(message)
    deleted_object_list = _empty_bucket_contents(context, args.all_versions, args.noncurrent_versions)
    staging.empty_staging_table(context, deleted_object_list)


def _empty_bucket_contents(context: object, all_versions: bool, noncurrent_versions: bool):
    """ Remove the dynamic content found in the S3 bucket
    
    Arguments
        context -- context to use
        all_versions -- whether to remove all versions of dynamic content
        noncurrent_versions -- whether to remove noncurrent versions of dynamic content
    """
    object_list = []
    deleted_object_list = []
    file_to_versions = content_bucket.list_all_versions(context)
    for content_key, available_versions in file_to_versions.items():
        for version in available_versions:
            if version.get('IsLatest'):
                if noncurrent_versions:
                    continue
            elif not (all_versions or noncurrent_versions):
                continue

            object = {'Key': content_key, 'VersionId': version.get('VersionId')}
            object_list.append(object)
            if len(object_list) == content_bucket.get_list_objects_limit():
                content_bucket.delete_objects_from_bucket(context, object_list)
                deleted_object_list.extend(object_list)
                object_list = []
    if len(object_list):
        content_bucket.delete_objects_from_bucket(context, object_list)
        deleted_object_list.extend(object_list)

    return deleted_object_list


def make_end_in_slash(file_name):
    return posixpath.join(file_name, '')


def _create_bucket_key(fileEntry):
    file_key = fileEntry.get('bucketPrefix', '')

    if len(file_key):
        file_key += '/'

    file_key += fileEntry.get('keyName', '')
    return file_key


def get_standalone_manifest_pak_key(context, manifest_path):
    return os.path.split(_get_path_for_standalone_manifest_pak(context, manifest_path))[1]


def get_standalone_manifest_key(context, manifest_path):
    return os.path.split(manifest_path)[1]


def add_manifest_pak_entry(context, manifest_path, filesList, manifest_version_id=None):
    """ When uploading all of the changed content within a top level manifest we're going to need to pak up the
    manifest itself and upload that as well.
    This method lets us simply append an entry about that manifest pak to the list of "changed content" so it all goes
    up in one pass.
    """
    manifest_object = {
        'keyName': get_standalone_manifest_pak_key(context, manifest_path),
        'localFolder': dynamic_content_settings.get_pak_folder(),
        'hash': hashlib.md5(open(manifest_path, 'rb').read()).hexdigest()
    }
    if manifest_version_id:
        manifest_path += f'.{manifest_version_id}'
    manifest_object['hash'] = hashlib.md5(open(manifest_path, 'rb').read()).hexdigest()
    manifest_object['contentHash'] = hashlib.md5(manifest_object['hash'].encode()).hexdigest()
    file_stat = os.stat(manifest_path)
    manifest_object['size'] = file_stat.st_size
    filesList.append(manifest_object)


def _create_manifest_bucket_key_map(context, manifest, manifest_path, manifest_version_id=''):
    if not manifest:
        return {}

    files_list = _get_files_list(context, manifest, 'Paks')

    add_manifest_pak_entry(context, manifest_path, files_list, manifest_version_id)
    _append_loose_manifest(context, files_list, manifest_path, manifest_version_id)

    return_map = {}
    for this_file in files_list:
        file_key = _create_bucket_key(this_file)
        return_map[file_key] = {
            'hash_metadata': hash_metadata(this_file.get('hash', ''), this_file.get('contentHash', '')),
            'version_id': this_file.get('versionId')
        }

    return return_map


def list_bucket_content(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path, args.manifest_version_id)
    contents_list = content_bucket.get_bucket_content_list(context)
    manifest_dict = _create_manifest_bucket_key_map(context, manifest, manifest_path, args.manifest_version_id)

    for this_bucket_item in contents_list:
        this_key = this_bucket_item.get('Key')
        if this_key in manifest_dict:
            synchronize_manifest_entry_with_bucket(context, this_key, manifest_dict[this_key])
            del manifest_dict[this_key]
    for remaining_key in manifest_dict.keys():
        show_manifest.new_local_key(remaining_key)
    _clear_versioned_manifest(manifest_path, args.manifest_version_id)


def command_list_file_versions(context: object, args: dict):
    """ CLI command to list all versions of a manifest or pak file found in the content bucket
    
        Arguments
            context -- context to use
            args -- command arguments
    """
    file_to_versions = content_bucket.list_all_versions(context)
    show_manifest.list_file_versions(args.file_name, file_to_versions.get(args.file_name, []))


def compare_bucket_content(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path, args.manifest_version_id)
    manifest_dict = _create_manifest_bucket_key_map(context, manifest, manifest_path, args.manifest_version_id)
    for this_key, this_manifest_entry in iteritems(manifest_dict):
        synchronize_manifest_entry_with_bucket(context, this_key, this_manifest_entry)
    _clear_versioned_manifest(manifest_path, args.manifest_version_id)


def synchronize_manifest_entry_with_bucket(context: object, file_key: str, manifest_entry: dict, deployment: str = '') -> Tuple[bool, str]:
    """ Synchronize the manifest entry with bucket. Returns a boolean which indicates whether 
        the manifest entry matches the existing s3 object and the version id of the S3 object
    
        Arguments
            context -- context to use
            file_key -- key of the bucket item
            manifest_entry -- information including hash metadata and version ID of the manifest entry
            deployment -- name of the deployment
    """
    bucket_hash_metadata, version_id = _head_bucket_current_item(context, file_key, deployment)
    manifest_hash_metadata = manifest_entry.get('hash_metadata', {})

    if not bucket_hash_metadata.hash:
        # Bucket item doesn't exist
        return False, version_id

    if manifest_hash_metadata.hash == bucket_hash_metadata.hash and (not bucket_hash_metadata.content_hash):
        # The local file doesn't actually change. Set content hash metadata of the bucket item if not exist
        bucket_hash_metadata.content_hash = manifest_hash_metadata.content_hash
        _update_bucket_hash_metadata(context, file_key, bucket_hash_metadata, deployment)
        manifest_entry['version_id'] = version_id

    if manifest_hash_metadata.content_hash == bucket_hash_metadata.content_hash:
        # Compare the content hash and reset the local manifest hash if the local file has the same content as the S3 object
        manifest_hash_metadata.hash = bucket_hash_metadata.hash
        manifest_entry['version_id'] = version_id
    show_manifest.content_hash_comparison_bucket(file_key, manifest_hash_metadata.content_hash, bucket_hash_metadata.content_hash)
    if content_bucket.content_versioning_enabled(context, deployment):
        show_manifest.content_version_comparison_bucket(file_key, manifest_entry.get('version_id'), version_id)
    return manifest_hash_metadata.content_hash == bucket_hash_metadata.content_hash, version_id


def synchronize_local_pak_with_bucket(context: object, pak_path: str, bucket_key: str, pak_hash_metadata: 'hash_metadata') -> Tuple[bool, str]:
    """ Synchronize the local pak with bucket. Returns a boolean which indicates whether 
        the local pak matches the existing s3 object and the version id of the object

    Arguments
        context -- context to use
        pak_path -- path to the local pak file
        bucket_key -- key of the bucket item
        pak_hash_metadata -- hash metadata of the pak file 
    """
    bucket_hash_metadata, version_id = _head_bucket_current_item(context, bucket_key)

    if not bucket_hash_metadata.hash:
        # Bucket item doesn't exist
        pak_hash_metadata.content_hash = pak_files.calculate_pak_content_hash(pak_path)
        return False, version_id

    if pak_hash_metadata.hash == bucket_hash_metadata.hash and bucket_hash_metadata.content_hash:
        pak_hash_metadata.content_hash = bucket_hash_metadata.content_hash
    else:
        pak_hash_metadata.content_hash = pak_files.calculate_pak_content_hash(pak_path)
        if pak_hash_metadata.hash == bucket_hash_metadata.hash:
            # The local file doesn't have content update. Set content hash metadata of the bucket item if not exist
            bucket_hash_metadata.content_hash = pak_hash_metadata.content_hash
            _update_bucket_hash_metadata(context, bucket_key, bucket_hash_metadata)

    print("Comparing {} vs Bucket {}".format(pak_hash_metadata.content_hash, bucket_hash_metadata.content_hash))
    return pak_hash_metadata.content_hash == bucket_hash_metadata.content_hash, version_id


def _get_hash_metadata_from_head_response(head_response: dict) -> 'hash_metadata':
    """ Get the hash metadata of the bucket item from the head response
    
    Arguments
        head_response -- response of the head call
    """
    standard_hash = head_response.get('Metadata', {}).get(_get_meta_hash_name(), '')
    content_hash = head_response.get('Metadata', {}).get(_get_meta_content_hash_name(), '')

    return hash_metadata(standard_hash, content_hash)


def _get_version_id_from_head_response(head_response: dict) -> str:
    """ Get the version ID of the bucket item from the head response
    
    Arguments
        head_response -- response of the head call
    """
    return head_response.get('VersionId')


class ProgressPercentage(object):
    def __init__(self, context, filename):
        self._filename = filename
        self._context = context
        self._size = float(os.path.getsize(filename))
        self._seen_so_far = 0
        self._lock = threading.Lock()
        self._last_print_percent = -1

    def __call__(self, bytes_amount):
        with self._lock:
            self._seen_so_far += bytes_amount
            percentage = (self._seen_so_far / self._size) * 100
            if round(percentage / 2) != round(self._last_print_percent / 2):
                self._last_print_percent = percentage
                self._context.view.update_percent_complete(self._last_print_percent)


def command_upload_manifest_content(context, args):
    staging_args = staging.parse_staging_arguments(args)
    upload_manifest_content(context, args.manifest_path, args.deployment_name, staging_args,
                            args.all, args.signing, args.invalidate_existing_files, args.replace)


def _append_loose_manifest(context, files_list, manifest_path, manifest_version_id=None):
    # Need to preserve sub-folder data
    manifest_path = ensure_posix_path(manifest_path)
    manifest_folder = dynamic_content_settings.get_manifest_folder()

    # Strip out any path information before the default manifest folder
    # to get relative sub-path under folder
    manifest_sub_path = manifest_path.partition(manifest_folder)[2]
    manifest_local_folder = dynamic_content_settings.get_manifest_folder()
    if len(manifest_sub_path):
        manifest_local_folder = manifest_local_folder + os.path.dirname(manifest_sub_path)

    manifest_object = {
        'keyName': os.path.split(manifest_path)[1],
        'localFolder': manifest_local_folder,
        'versionId': manifest_version_id
    }
    if manifest_version_id:
        manifest_path += f'.{manifest_version_id}'
    manifest_object['hash'] = hashlib.md5(open(manifest_path, 'rb').read()).hexdigest()
    manifest_object['contentHash'] = manifest_object['hash']
    file_stat = os.stat(manifest_path)
    manifest_object['size'] = file_stat.st_size
    files_list.append(manifest_object)


# 1 - Build new paks to ensure our paks are up to date and our manifest reflects the latest changes
# 2 - Sychronize with bucket: Upload unmatched files and update the manifest files
# 3 - Set the staging status for each uploaded file
def upload_manifest_content(context, manifest_path, deployment_name, staging_args,
                            upload_all=False, do_signing=False, invalidate_existing_files=False, replace=False):
    build_new_paks(context, manifest_path, upload_all, False)
    uploaded_files, unchanged_files, manifest_path = synchronize_with_bucket(context, manifest_path, deployment_name, upload_all, do_signing,
                                                                             invalidate_existing_files)

    for this_file, file_info in iteritems(uploaded_files):
        # Set the staging status for all the new/updated content
        staging_args['Signature'] = file_info.get('Signature')
        staging_args['Size'] = file_info.get('Size')
        staging_args['Hash'] = file_info.get('Hash')
        staging_args['ContentHash'] = file_info.get('ContentHash')
        staging_args['Parent'] = file_info.get('Parent')
        staging_args['ParentVersionId'] = uploaded_files.get(staging_args['Parent'], {}).get('VersionId')
        staging.set_staging_status(this_file, context, staging_args, deployment_name, file_info.get('VersionId'))

    staging_args = {}
    for this_file, file_info in iteritems(unchanged_files):
        # For any matched item which is shared by the new manifest version, we should only 
        # update its parent version ID list and keep its original staging status
        staging_args['ParentVersionId'] = uploaded_files.get(file_info['Parent'], {}).get('VersionId')
        staging.set_staging_status(this_file, context, staging_args, deployment_name, file_info.get('VersionId'))

    if replace:
        _remove_old_versions(context, deployment_name, uploaded_files.keys())


# 1 - Check each item in our local manifest against a HEAD call to get its content hash and standard hash metadata
# 2 - For each matched item by content hash, replace its standard hash in the local manifest with the standard hash metadata of the same named S3 object
# 3 - Update the version ID of each matched pak (returned from the HEAD call) in our manifest
# 4 - Upload each unmatched pak file by content hash and set its metadata in S3
# 5 - Update the version ID of each unmatched pak (returned from the upload request) in the manifest 
# 6 - Upload the manifest (In pak and loose)
def synchronize_with_bucket(context: object, manifest_path: str, deployment_name: str,
                            ignore_file_check: bool, do_signing: bool, invalidate_existing_files: bool) -> Tuple[list, list, str]:
    """ Synchronize with the content bucket to upload unmatched content and update manifest files. The workflow should look like below:
  
    Arguments
        context -- context to use
        manifest_path -- relative path of the manifest
        deployment_name -- name of the deployment
        ignore_file_check -- whether to ignore the file check
        do_signing -- whether to add file signatures to the content table
        invalidate_existing_files -- Whether to invalidate same named s3 items during the upload
    """
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    paks_list = _get_files_list(context, manifest, 'Paks')
    standalone_manifest_list = []
    _append_loose_manifest(context, standalone_manifest_list, manifest_path)
    add_manifest_pak_entry(context, manifest_path, standalone_manifest_list)
    files_list = paks_list + standalone_manifest_list
    versioned = content_bucket.content_versioning_enabled(context, deployment_name)
    uploaded_files = {}
    unchanged_files = {}

    manifest_hash = ''
    standalone_manifest_pak_key = get_standalone_manifest_pak_key(context, manifest_path)
    loose_manifest_key = get_standalone_manifest_key(context, manifest_path)
    for file in files_list:
        parent = '' if file['keyName'] in [standalone_manifest_pak_key, loose_manifest_key] else standalone_manifest_pak_key
        if file['keyName'] == loose_manifest_key:
            # Save the local manifest since the hash of matched content might be updated after the sync
            _save_content_manifest(context, manifest_path, manifest)
            # Recalculate hash metadata of the local manifest
            file['contentHash'] = file['hash'] = hashlib.md5(open(manifest_path, 'rb').read()).hexdigest()
            manifest_hash = file['hash']
            create_standalone_manifest_pak(context, manifest_path)
        elif file['keyName'] == standalone_manifest_pak_key:
            # Recalculate hash metadata of the standalone manifest pak
            file['hash'] = manifest_hash
            file['contentHash'] = hashlib.md5(manifest_hash.encode()).hexdigest()

        file_hash_metadata = hash_metadata(file.get('hash', ''), file.get('contentHash', ''))
        matched, version_id = synchronize_manifest_entry_with_bucket(context, file['keyName'], {'hash_metadata': file_hash_metadata}, deployment_name)
        if matched:
            # Reset the hash of matched files to line up with our current manifest in the bucket
            file['hash'] = file_hash_metadata.hash
            if versioned:
                file['versionId'] = version_id
            else:
                file.pop('versionId', None)
            if (not staging.signing_status_changed(context, file['keyName'], do_signing, file.get('versionId'))) and (not ignore_file_check):
                # File matches the existing s3 item. Don't need to upload it unless customers force to do that
                unchanged_files[file['keyName']] = {'Parent': parent, 'VersionId': version_id}
                continue
        _upload_unmatched_file(context, file, invalidate_existing_files, deployment_name)
        updated_version_id = _get_file_version_id(context, file['keyName'], deployment_name)
        if versioned:
            file['versionId'] = updated_version_id
        else:
            file.pop('versionId', None)
        uploaded_files[file['keyName']] = _create_upload_info(context, file, parent, do_signing)

    return uploaded_files, unchanged_files, manifest_path


def _remove_old_versions(context: object, deployment_name: str, files_to_remove: list):
    """ 
    Remove all the noncurrent versions of files
  
    Arguments
        context -- context to use
        deployment_name -- name of the deployment
        files_to_remove -- list of files to remove the old versions for
    """
    s3 = context.aws.client('s3')
    bucket_name = content_bucket.get_content_bucket_by_name(context, deployment_name)
    file_to_versions = content_bucket.list_all_versions(context, deployment_name)
    for file_key in files_to_remove:
        versions = file_to_versions.get(file_key, [])
        for version in versions:
            if version.get('IsLatest'):
                continue

            version_id = version.get('VersionId')
            print(f'Delete item from S3. Key: {file_key} VersionId: {version_id}')
            try:
                response = s3.delete_object(
                    Bucket=bucket_name,
                    Key=file_key,
                    VersionId=version_id
                )
            except Exception as e:
                print(f'Failed to delete item from S3. Key: {file_key} VersionId: {version_id} Error: {str(e)}')
                continue
            staging.remove_entry(context, file_key, version_id)


def _get_file_version_id(context: object, key: str, deployment_name: str) -> str:
    """ Get the version id of an s3 object
    
    Arguments
        context -- context to use
        bucket_name -- name of the s3 bucket
        key -- key of the s3 object
    """
    bucket_hash_metadata, version_id = _head_bucket_current_item(context, key, deployment_name)
    return version_id


def _upload_unmatched_file(context, file, invalidate_existing_files, deployment_name=''):
    """ Upload unmatched dynamic content to the s3 bucket
    
    Arguments
        context -- context to use
        bucket_name -- name of the s3 bucket
        file -- file to upload
        invalidate_existing_files -- Whether to invalidate same named s3 items during the upload
    """
    file_path = _get_path_for_file_entry(context, file, context.config.game_directory_name)
    if not file['hash'] or not file['keyName']:
        show_manifest.skipping_invalid_file(file_path)
        return
    show_manifest.found_updated_item(file['keyName'])
    context.view.found_updated_item(file['keyName'])
    file_hash_metadata = hash_metadata(file.get('hash', ''), file.get('contentHash', ''))
    _do_file_upload(context, file_path, file['keyName'], file_hash_metadata, invalidate_existing_files, deployment_name)


def _create_upload_info(context, file, parent, do_signing):
    """ Gather the upload info for writing to the staging settings table later
    
    Arguments
        context -- context to use
        file -- file that has been uploaded to s3
        parent -- parent of the uploaded file
        do_signing -- Whether to add file signatures to the content table for client side verification
    """
    upload_info = {}
    if do_signing:
        signature = signing.get_file_signature(context, _get_path_for_file_entry(context, file, context.config.game_directory_name))
        upload_info['Signature'] = signature
    upload_info['Size'] = file.get('size')
    upload_info['Hash'] = file.get('hash')
    upload_info['ContentHash'] = file.get('contentHash', '')
    upload_info['VersionId'] = file.get('versionId')
    upload_info['Parent'] = parent
    return upload_info


def _get_meta_hash_name() -> str:
    """ Get the name of the hash metadata """
    return 'hash'


def _get_meta_content_hash_name() -> str:
    """ Get the name of the content hash metadata """
    return 'contenthash'


def _do_file_upload(context, this_file_path, this_key, file_hash_metadata, invalidate_existing_files=False, deployment_name=''):
    bucket_name = content_bucket.get_content_bucket_by_name(context, deployment_name)
    s3 = context.aws.client('s3')
    if invalidate_existing_files:
        try:
            s3.get_object(
                Bucket=bucket_name,
                Key=this_key
            )
            cloudfront.create_invalidation(context, this_key, this_key + file_hash_metadata.hash)
        except ClientError as e:
            if e.response['Error']['Code'] == 'NoSuchKey':
                # File doesn't exist in the s3 bucket. Consider it as a new file.
                pass

    config = TransferConfig(
        max_concurrency=10,
        num_download_attempts=10,
    )

    transfer = S3Transfer(s3, config)
    transfer.upload_file(this_file_path, bucket_name, this_key,
                         callback=ProgressPercentage(context, this_file_path),
                         extra_args={'Metadata': {
                             _get_meta_hash_name(): file_hash_metadata.hash,
                             _get_meta_content_hash_name(): file_hash_metadata.content_hash
                         }})
    show_manifest.done_uploading(this_file_path)
    context.view.done_uploading(this_file_path)


def _get_cache_game_path(context, game_directory):
    return os.path.join('cache', game_directory)


def _get_cache_platform_game_path(context, platform, game_directory):
    return_path = _get_cache_game_path(context, game_directory)
    # Asset process writes to lowercased game name directory, so ensure path is compatible with non-windows systems
    return_path = os.path.join(return_path, _get_path_for_platform(platform), game_directory.lower())
    return_path = make_end_in_slash(return_path)
    return return_path


def _get_path_for_platform(aliasName):
    if aliasName is None or aliasName == '' or aliasName == 'shared':
        if _platform == 'win32':
            return 'pc'
        if _platform == 'darwin':
            return 'osx_gl'
        if _platform == 'linux' or _platform == 'linux2':
            return 'linux'
        return _platform
    return aliasName


def _get_full_platform_cache_game_path(context, platform, game_directory):
    cache_game_path = _get_cache_game_path(context, game_directory)
    platform_cache_game_path = _get_cache_platform_game_path(context, platform, game_directory)

    full_platform_cache_game_path = os.path.join(context.config.root_directory_path, platform_cache_game_path)
    if not os.path.isdir(full_platform_cache_game_path):
        full_platform_cache_game_path = os.path.join(context.config.root_directory_path, cache_game_path)
    return full_platform_cache_game_path


def _get_game_root_for_file_entry(context, manifestEntry, game_directory):
    return_path = context.config.root_directory_path
    if manifestEntry.get('cacheRoot', {}) == '@assets@':
        return os.path.join(return_path,
                            _get_cache_platform_game_path(context,
                                                          manifestEntry.get('platformType', None),
                                                          game_directory))
    return os.path.join(return_path, game_directory)


def _get_root_for_file_entry(context, manifestEntry, game_directory):
    return_path = context.config.root_directory_path
    if manifestEntry.get('cacheRoot', {}) == '@assets@':
        return_path = os.path.join(return_path,
                                   _get_cache_platform_game_path(context,
                                                                 manifestEntry.get('platformType', None),
                                                                 game_directory))
    else:
        return_path = os.path.join(return_path, game_directory)
    local_folder = manifestEntry.get('localFolder')
    if not local_folder:
        raise HandledError('Entry has no localFolder: {}'.format(manifestEntry))
    return_path = os.path.join(return_path, local_folder)
    return return_path


def _get_path_for_file_entry(context, manifestEntry, game_directory):
    file_path = os.path.join(_get_root_for_file_entry(context, manifestEntry, game_directory),
                             manifestEntry['keyName'])
    return_path = ensure_posix_path(os.path.normpath(file_path))
    return return_path


# Returns the relative pak path for a given entry such as /DynamicContent/Paks/carassets.shared.pak
def _get_pak_for_entry(context, fileEntry, manifest_path):
    pak_path = fileEntry.get('pakFile')

    if not pak_path:
        return None
    return ensure_posix_path(pak_path)


# manifest_path - full path and file name of manifest.json
# manifest - full manifest dictionary object
# pak_all - Disregard whether data appears to have been updated and return all of the paks with their content lists
def _get_updated_local_content(context, manifest_path, manifest, pak_all):
    # sorts manifest files by pak_folder_path
    filesList = _get_files_list(context, manifest, 'Files')
    thisFile = {}
    context.view.finding_updated_content(manifest_path)
    returnData = {}
    updated_paks = set()
    for thisFile in filesList:
        this_file_path = _get_path_for_file_entry(context, thisFile, context.config.game_directory_name)
        if not os.path.isfile(this_file_path):
            context.view.invalid_file(this_file_path)
            thisFile['hash'] = ''
            thisFile['size'] = None
            continue

        relative_pak_folder = _get_pak_for_entry(context, thisFile, manifest_path)

        if not relative_pak_folder:
            continue

        if not returnData.get(relative_pak_folder):
            returnData[relative_pak_folder] = []
        # Add an entry for every file regardless of whether it's updated - we'll remove the paks that
        # have no updates at the end
        returnData[relative_pak_folder].append(thisFile)

        if relative_pak_folder in updated_paks:
            # We know this pak needs an update already
            continue

        hex_return = hashlib.md5(open(this_file_path, 'rb').read()).hexdigest()
        manifestHash = thisFile.get('hash', '')
        context.view.hash_comparison_disk(this_file_path, manifestHash, hex_return)

        if hex_return != manifestHash:
            updated_paks.add(relative_pak_folder)

    # pak_all is just a simple way to say give me back all of the data in the expected pak_file, list of files format
    # regardless of updates.  In most cases we want to strip out the paks we haven't found any updates for
    if not pak_all:
        # In Python 2.x calling keys makes a copy of the key that you can iterate over while modifying the dict.
        # This doesn't work in Python 3.x because keys returns an iterable which means that it's a view on the
        # dictionary's keys directly.
        paks = [key for key, value in returnData.items()]
        for this_pak in paks:
            output_pak_path = get_pak_game_folder(context, this_pak)
            pak_exists = os.path.isfile(output_pak_path)

            if pak_exists and this_pak not in updated_paks:
                show_manifest.skipping_unmodified_pak(this_pak)
                del returnData[this_pak]

    return returnData


def _get_path_for_standalone_manifest_pak(context, manifest_path):
    file_name = posixpath.sep.join(manifest_path.split(os.path.sep))
    head, file_name = posixpath.split(file_name)
    file_name = file_name.replace(dynamic_content_settings.get_manifest_extension(), dynamic_content_settings.get_manifest_pak_extension())
    return_path = os.path.join(dynamic_content_settings.get_pak_game_folder(context.config.game_directory_path), file_name)
    return return_path


def create_standalone_manifest_pak(context, manifest_path):
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    full_path = ensure_posix_path(os.path.join(context.config.root_directory_path, manifest_path))

    archiver = pak_files.PakFileArchiver()
    pak_folder_path = _get_path_for_standalone_manifest_pak(context, full_path)
    files_to_pak = []
    base_content_path = ensure_posix_path(
        os.path.join(context.config.root_directory_path, context.config.game_directory_name))
    file_pair = (full_path, base_content_path)
    files_to_pak.append(file_pair)
    archiver.archive_files(files_to_pak, pak_folder_path)


def get_path_for_manifest_platform(context, manifest_path, platform):
    manifest_root, manifest_name = os.path.split(manifest_path)
    if len(manifest_name):
        manifest_name = manifest_name.split('.json', 1)[0]
    else:
        manifest_name = manifest_path

    pak_folder_path = os.path.join(dynamic_content_settings.get_pak_folder(), manifest_name + '.' + platform + '.pak')
    if len(pak_folder_path) and pak_folder_path[0] == '/':
        pak_folder_path = pak_folder_path[1:]
    return pak_folder_path


def get_pak_game_folder(context, relative_folder_path):
    pak_folder_path = ensure_posix_path(os.path.join(context.config.game_directory_path, relative_folder_path))
    return pak_folder_path


def command_build_new_paks(context, args):
    build_new_paks(context, args.manifest_path, args.all)


def build_new_paks(context, manifest_path, pak_All=False, create_manifest_pak=True):
    # TODO: add variable for a pak name, and add the pak name to the file entry in the manifest
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    updatedContent = _get_updated_local_content(context, manifest_path, manifest, pak_All)
    for relative_folder_path, files in iteritems(updatedContent):
        # currently this command is only supported on windows
        archiver = pak_files.PakFileArchiver()
        files_to_pak = []
        pak_folder_path = get_pak_game_folder(context, relative_folder_path)
        for file in files:
            file['pakFile'] = relative_folder_path
            file_pair = (_get_path_for_file_entry(context, file, context.config.game_directory_name),
                         _get_game_root_for_file_entry(context, file, context.config.game_directory_name))
            files_to_pak.append(file_pair)
            print(file_pair)
        archiver.archive_files(files_to_pak, pak_folder_path)
    _save_content_manifest(context, manifest_path, manifest)
    _update_file_hashes(context, manifest_path, manifest)

    if create_manifest_pak:
        create_standalone_manifest_pak(context, manifest_path)


def upload_folder_command(context, args):
    staging_args = staging.parse_staging_arguments(args)
    upload_folder(context, args.folder, args.bundle_type, args.deployment_name, staging_args, args.signing, args.invalidate_existing_files)


def upload_folder(context, folder, bundle_type, deployment_name, staging_args, do_signing=False, invalidate_existing_files=False):
    """Upload all of the bundles in a specific folder - no manifest necessary"""
    folder_path = folder if os.path.isabs(folder) else os.path.abspath(folder)
    bundles = glob.glob(os.path.join(folder_path, '*' + os.path.extsep + bundle_type))

    if len(bundles) == 0:
        print('No bundles of type {} found at {}'.format(bundle_type, folder_path))
        return

    print('Found {} potential {} bundles at {}'.format(len(bundles), bundle_type, folder_path))
    bucketName = content_bucket.get_content_bucket_by_name(context, deployment_name)

    uploaded_files = {}
    print('Comparing against bucket {}:'.format(bucketName))
    for thisBundle in bundles:
        thisKey = os.path.split(thisBundle)[1]
        thisHash = hashlib.md5(open(thisBundle, 'rb').read()).hexdigest()
        file_stat = os.stat(thisBundle)
        thisSize = file_stat.st_size
        print('Found bundle {}'.format(thisBundle))
        print('Size {} Hash {}'.format(thisSize, thisHash))
        bundle_hash_metadata = hash_metadata(hash=thisHash)
        matched, version_id = synchronize_local_pak_with_bucket(context, thisBundle, thisKey, bundle_hash_metadata)
        if matched:
            print('Bucket entry matches, skipping')
            continue

        print('Uploading {} as {}'.format(thisBundle, thisKey))
        _do_file_upload(context, thisBundle, thisKey, bundle_hash_metadata, invalidate_existing_files, deployment_name)
        version_id = _get_file_version_id(context, thisKey, deployment_name)
        upload_info = {}

        if do_signing:
            this_signature = signing.get_file_signature(context, _get_path_for_file_entry(context, thisBundle,
                                                                                          context.config.game_directory_name))
            upload_info['Signature'] = this_signature
        upload_info['Size'] = thisSize
        upload_info['Hash'] = bundle_hash_metadata.hash
        upload_info['ContentHash'] = bundle_hash_metadata.content_hash
        upload_info['VersionId'] = version_id
        uploaded_files[thisKey] = upload_info

    if staging_args is not None:
        for thisFile, file_info in iteritems(uploaded_files):
            staging_args['Signature'] = file_info.get('Signature')
            staging_args['Size'] = file_info.get('Size')
            staging_args['Hash'] = file_info.get('Hash')
            staging_args['ContentHash'] = file_info.get('ContentHash')
            staging_args['Parent'] = ''
            staging.set_staging_status(thisFile, context, staging_args, deployment_name, file_info.get('VersionId'))


class hash_metadata(object):
    """ Object used to store hash and hash content metadata """

    def __init__(self, hash='', content_hash=''):
        self.hash = hash
        self.content_hash = content_hash

    @property
    def hash(self):
        return self._hash

    @property
    def content_hash(self):
        return self._content_hash

    @hash.setter
    def hash(self, value):
        self._hash = value

    @content_hash.setter
    def content_hash(self, value):
        self._content_hash = value
