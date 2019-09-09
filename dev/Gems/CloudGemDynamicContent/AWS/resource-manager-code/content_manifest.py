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
from resource_manager.errors import HandledError
from boto3.s3.transfer import S3Transfer
from boto3.s3.transfer import TransferConfig
import os
import hashlib
import sys
import Queue
import threading
import glob
import re
from collections import defaultdict
import posixpath
import staging
import signing
import dynamic_content_settings
from sys import platform as _platform
import show_manifest
import pak_files
from functools import wraps

def list_manifests(context, args):
    # this will need to look for manifest references and build a tree
    # new code should give the folder directly, so remove the path.dirname when possible
    manifest_path = os.path.normpath(os.path.dirname(_get_default_manifest_path(context)))
    manifests = glob.glob(os.path.join(manifest_path, '*' + os.path.extsep + 'json'))
    context.view.list_manifests(manifests)

def list(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    filesList = _get_files_list(context, manifest, args.section)
    if args.file_name:
        filesList = [ thisEntry for thisEntry in filesList if entry_matches_file(thisEntry, args.file_name, args.platform_type)] 

    context.view.show_manifest_file(filesList)

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
def gui_list(context, args, pak_status = None):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    sections = {}
    sections['Files'] = _get_files_list(context, manifest, 'Files')
    sections['Paks'] = _get_files_list(context, manifest, 'Paks')
    sections['Platforms'] = _get_platforms_list(context, manifest)
    sections['PakStatus'] = pak_status
    context.view.show_manifest_file(sections)

def validate_manifest_name(manifest_name):
    regex_str = '^[-0-9a-zA-Z!_][-0-9a-zA-Z!_.]*$'
    return re.match(regex_str, manifest_name) != None

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
        add_file_entry(context, manifest_path, file_name, 'Files', platform_type = platformType)
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
        file_name = os.path.join(new_file.get('localFolder'), new_file.get('keyName')).replace('\\','/')
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
        if _compare_path(os.path.join(existing_file['localFolder'], existing_file['keyName']), file_to_remove['keyName']) and existing_file['platformType'] == file_to_remove['platformType']:
            pak_status[os.path.basename(existing_file['pakFile'])] = 'outdated'
            del existing_file['pakFile']
    manifest['Files'] = file_list
    _save_content_manifest(context, manifest_path, manifest)
    gui_list(context, args, pak_status)

def gui_pak_and_upload(context, args):
    if not gui_is_stack_configured(context):
        context.view.gui_signal_upload_complete()
        return
    command_upload_manifest_content(context, args)
    pak_status = {}
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    pak_list = _get_files_list(context, manifest, 'Paks')
    for pak in pak_list:
        pak_status[pak['keyName']] = 'match'
    gui_list(context, args, pak_status)
    context.view.show_local_file_diff([])   # all files up to date, so clear status on all of them
    context.view.gui_signal_upload_complete()

@stack_required
def gui_get_bucket_status(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    contentsList = _get_bucket_content_list(context)
    manifestDict = _create_manifest_bucket_key_map(context, manifest, manifest_path)
    bucketDiffData = {'new': [], 'outdated' : [], 'match' : []}
    for thisBucketItem in contentsList:
        thisKey = thisBucketItem.get('Key')
        if thisKey in manifestDict:
            manifestItemHash = manifestDict[thisKey]
            bucketItemHash = _get_bucket_item_hash(thisBucketItem)
            if(manifestItemHash == bucketItemHash):
                bucketDiffData['match'].append(thisKey)
            else:
                bucketDiffData['outdated'].append(thisKey)
            del manifestDict[thisKey]
    for remainingKey in manifestDict.keys():
        bucketDiffData['new'].append(remainingKey)
    context.view.show_bucket_diff(bucketDiffData)

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
    context.view.generate_keys_completed();

@stack_required
def gui_get_local_file_status(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    updated_content = _get_updated_local_content(context, manifest_path, manifest, False)
    local_status = []
    for platform, files in updated_content.iteritems():
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
    os.remove(manifest_path) # TODO offer to delete associated paks?

def is_manifest_entry(thisEntry):
    localFolder = thisEntry.get('localFolder')
    localFolder = make_end_in_slash(localFolder)
    
    return localFolder.endswith(dynamic_content_settings.get_manifest_folder())


def validate_add_key_name(file_name):
    if len(file_name) >= 100: #AWS Limit
        raise HandledError('File name too long')
    regex_str = '^[-0-9a-zA-Z!_\.\(\)/\\\]*$'
    if not re.match(regex_str, file_name):
        raise HandledError('File does not match naming rules')

def command_new_manifest(context, args):
    if not validate_manifest_name(args.manifest_name):
        raise HandledError('Invalid manifest name')
    manifest_path = determine_manifest_path(context, args.manifest_path)
    manifest_dir_path = os.path.normpath(os.path.dirname(manifest_path))
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
    if target_platforms == None:
        target_platforms = []
    change_target_platforms(context, target_platforms, manifest_path)

    context.view.update_target_platforms(manifest_path)

def command_add_pak(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    platform_type = args.platform_type
    new_file_name = args.pak_name + os.path.extsep + platform_type + os.path.extsep + 'pak'
    _add_pak_to_manifest(context, os.path.join(dynamic_content_settings.get_pak_folder(), new_file_name), manifest_path, manifest, platform_type)

def entry_matches_platform(this_entry, platform_type):
    this_platform = this_entry.get('platformType')

    if this_platform and this_platform != 'shared' and platform_type != 'shared' and platform_type != this_platform:
        print 'Platforms do not match {} vs {}'.format(this_platform, platform_type)
        return False
    return True           

def entry_matches_file(this_entry, file_name, file_platform):
    local_folder = this_entry.get('localFolder')
    key_name = this_entry.get('keyName')
    platform_type = this_entry.get('platformType')

    if local_folder == '.' and key_name == file_name and platform_type == file_platform:
        return True

    existing_file_name = os.path.join(local_folder, key_name)
    existing_file_name = existing_file_name.replace('\\','/')
    print 'Comparing {} vs {}'.format(existing_file_name, file_name)
    return (existing_file_name == file_name and platform_type == file_platform)

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
    file_entry = {}
    for file_entry in file_list:
        if not entry_matches_platform(file_entry, pak_platform_type):
            continue

        if entry_matches_file(file_entry, file_name, file_platform):
            file_entry['pakFile'] = pak_file
            file_entry['hash'] = '' # Need to be sure to add this to the pak next update
            platform_name = file_entry.get('platformType')
            file_found = True
            break
    if not file_found:
        raise HandledError('No matching file found {} platform {}'.format(file_name,pak_platform_type))
    manifest['Files'] = file_list
    _save_content_manifest(context, manifest_path, manifest)

def command_add_file_entry(context, args):
    add_file_entry(context, args.manifest_path, args.file_name, args.file_section, None, args.cache_root, args.bucket_prefix, args.output_root, args.platform_type)
      
def add_file_entry(context, manifest_path, file_path, manifest_section, pakFileEntry = None, cache_root = None, bucket_prefix = None, output_root = None, platform_type = None):
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    file_section = manifest_section
    if file_section is None:
        file_section = 'Files'
    thisFile = {}
    if file_path is None:
        raise HandledError('No file name specified')
    file_path = file_path.replace('\\','/')
        
    namePair = os.path.split(file_path)
    fileName = namePair[1]
    localPath = namePair[0]
    if len(localPath) and localPath[0]=='/':
        localPath = localPath[1:]
    if fileName is None:
        raise HandledError('No file name specified')
    validate_add_key_name(fileName)
    thisFile['keyName'] = fileName
    if not(localPath and len(localPath)):
        localPath = '.'
    thisFile['cacheRoot'] = cache_root or '@assets@'
    thisFile['localFolder'] = localPath
    thisFile['bucketPrefix'] = bucket_prefix or ''
    thisFile['outputRoot'] = output_root or '@user@'
    thisFile['platformType'] = platform_type or ''
    if pakFileEntry is not None:
        thisFile['pakFile'] = pakFileEntry or ''
    thisFile['isManifest'] = is_manifest_entry(thisFile)

    existingList = _get_files_list(context,manifest,file_section)
    filesList = [ thisEntry for thisEntry in existingList if not (thisEntry.get('keyName') == fileName and thisEntry.get('localFolder','.') == localPath and thisEntry.get('platformType') == platform_type)] 

    filesList.append(thisFile)
    manifest[file_section] = filesList
    _save_content_manifest(context, manifest_path, manifest)

def _get_relative_path_after_game_dir(context, pak_path):
    replace_path = pak_path.replace('\\','/')
    try:
        return replace_path.split(context.config.game_directory_name + '/')[1]
    except:
        return replace_path

def _add_pak_to_manifest(context, pak_path, manifest_path, manifest, platform_name):
    paksList = _get_paks_list(context, manifest)
    potentialPakFileEntryPath = _get_relative_path_after_game_dir(context, pak_path)
    pakAlreadyListedInManifest = False
    for pakEntryToCheck in paksList:
        if pakEntryToCheck.get('pakFile') == potentialPakFileEntryPath:
            pakAlreadyListedInManifest = True
            break
    if not pakAlreadyListedInManifest:
        add_file_entry(context, manifest_path, potentialPakFileEntryPath, 'Paks', potentialPakFileEntryPath, './', platform_type= platform_name)

def _remove_file_entry_from_section(context,file_name, platform, manifest_path, manifest, section):
    if section is None:
        raise HandledError('No section specified to remove file entry from')
        
    filesList = _get_files_list(context,manifest, section)
    thisFile = {}
    if file_name is None:
        raise HandledError('No file name specified')
    namePair = os.path.split(file_name)
    fileName = namePair[1]
    localPath = namePair[0]
    foundEntry = False
    for thisFile in filesList:
        if thisFile['keyName'] == fileName and thisFile['localFolder'] == localPath and thisFile['platformType'] == platform:
            filesList.remove(thisFile)
            foundEntry = True
    manifest[section] = filesList
    if foundEntry:
        _save_content_manifest(context, manifest_path, manifest)
    return foundEntry

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

    foundEntry = _remove_file_entry_from_section(context, args.file_name, args.platform_type, manifest_path, manifest, 'Files')
    if not foundEntry:
        foundEntry = _remove_file_entry_from_section(context, args.file_name, args.platform_type, manifest_path, manifest, 'Paks')  
    if not foundEntry:
        raise HandledError('Key not found {} with platform type {}'.format(args.file_name, args.platform_type))

def _get_files_list(context, manifest, section = None):
    if manifest is None:
        raise HandledError('No manifest data loaded')

    if section is not None:
        filesList = manifest.get(section)
        if filesList is None:

            if section not in ['Files', 'Paks']:

                raise HandledError('No section {} found in manifest'.format(section))

                

            filesList = []

    else:
        filesList = []
        filesList.extend(manifest.get('Files',[]))
        filesList.extend(manifest.get('Paks',[]))
        
    return filesList

def _get_platforms_list(context, manifest):
    if manifest is None:
        raise HandledError('No manifest data loaded')

    return manifest.get('Metadata',{}).get('Platforms',[])


def _get_paks_list(context, manifest):
    if manifest is None:
        raise HandledError('No manifest data loaded')
    paksList = manifest.get('Paks', [])
    if paksList is None:
        raise HandledError('No Paks list found in manifest')
    return paksList

def _get_default_manifest_path(context):
    base_path = dynamic_content_settings.get_manifest_game_folder(context.config.game_directory_path)
    return_path = os.path.join(base_path,dynamic_content_settings.get_default_manifest_name())
    return return_path

def determine_manifest_path(context, providedName):
    if providedName == None:
        manifest_path = _get_default_manifest_path(context)
    else:
        manifest_path = os.path.join(dynamic_content_settings.get_manifest_game_folder(context.config.game_directory_path),providedName)
    return manifest_path

def _get_path_and_manifest(context, providedName):
    manifest_path = determine_manifest_path(context, providedName)
    manifest = context.config.load_json(manifest_path)
    return manifest_path, manifest

def command_update_file_hashes(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    _update_file_hashes(context, manifest_path, manifest)

def _update_file_hash_section(context, manifest_path, manifest, section):
    if section is None:
        raise HandledError('No specified to update hashes for')
        
    filesList = _get_files_list(context, manifest, section)
    thisFile = {}
    show_manifest.updating_file_hashes(manifest_path)
    files_updated = False
    for thisFile in filesList:
        this_file_path = _get_path_for_file_entry(context, thisFile, context.config.game_directory_name)
        if not os.path.isfile(this_file_path):
            show_manifest.invalid_file(this_file_path)
            thisFile['hash'] = ''
            continue
        hex_return = hashlib.md5(open(this_file_path,'rb').read()).hexdigest()
        manifestHash = thisFile.get('hash','')
        if hex_return != manifestHash:
            files_updated = True
            
        show_manifest.hash_comparison_disk(this_file_path, manifestHash, hex_return)
        thisFile['hash'] = hex_return
    manifest[section] = filesList
    return files_updated
    
def _update_file_hashes(context, manifest_path, manifest):
    need_save = _update_file_hash_section(context, manifest_path, manifest, 'Files')
    need_save |= _update_file_hash_section(context, manifest_path, manifest, 'Paks')
    if need_save:
        _save_content_manifest(context, manifest_path, manifest)

def _save_content_manifest(context, filePath, manifestData):
    context.config.validate_writable(filePath)
    context.config.save_json(filePath, manifestData)

def _get_content_bucket(context):
    return _get_content_bucket_by_name(context, context.config.default_deployment, dynamic_content_settings.get_default_resource_group(), dynamic_content_settings.get_default_bucket_name())

def _get_content_bucket_by_name(context, deployment_name, resource_group_name = dynamic_content_settings.get_default_resource_group(), bucket_name = dynamic_content_settings.get_default_bucket_name()):
    '''Returns the resource id of the content bucket.'''
    if deployment_name is None:
        deployment_name = context.config.default_deployment
  
    stack_id = context.config.get_resource_group_stack_id(deployment_name , resource_group_name, optional=True)
    bucketResource = context.stack.get_physical_resource_id(stack_id, bucket_name)
    return bucketResource

def _get_bucket_content_list(context):
    s3 = context.aws.client('s3')
    bucketName = _get_content_bucket(context)
    nextMarker = 0
    contentsList = []
    while True:
        try:
            res = s3.list_objects(
                Bucket = bucketName,
                Marker = str(nextMarker)
            )
            thisList = res.get('Contents',[])
            contentsList += thisList
            if len(thisList) < get_list_objects_limit():
                break
            nextMarker += get_list_objects_limit()
        except Exception as e:
            raise HandledError(
                'Could not list_objects on '.format(
                    bucket = bucketName,
                ),
                e
            )
    return contentsList

def _send_bucket_delete_list(context, objectList):
    s3 = context.aws.client('s3')
    bucketName = _get_content_bucket(context)
    try:
        res = s3.delete_objects(
            Bucket = bucketName,
            Delete = { 'Objects': objectList }
        )
    except Exception as e:
        raise HandledError(
            'Could not delete_objects on '.format(
                bucket = bucketName,
            ),
            e
        )

def command_empty_content(context, args):
    _empty_bucket_contents(context)
    staging.empty_staging_table(context)
    
def _empty_bucket_contents(context):
    contentsList = _get_bucket_content_list(context)
    listLength = len(contentsList)
    curIndex = 1
    objectList = []
    
    for thisContent in contentsList:
        objectList.append({ 'Key': thisContent.get('Key',{})})
        if len(objectList) == get_list_objects_limit():
            _send_bucket_delete_list(context,objectList)
            objectList = []
    if len(objectList):
        _send_bucket_delete_list(context,objectList)

def make_end_in_slash(file_name):
    return posixpath.join(file_name,'')
    
def _create_bucket_key(fileEntry):

    fileKey = fileEntry.get('bucketPrefix','')

    if len(fileKey):

        fileKey += '/'

    fileKey += fileEntry.get('keyName','')

    return fileKey


def get_standalone_manifest_key(context, manifest_path):

    return os.path.split(_get_path_for_standalone_manifest_pak(context, manifest_path))[1]


# When uploading all of the changed content within a top level manifest we're going to need to pak up the manifest itself and upload that as well
# This method lets us simply append an entry about that manifest pak to the list of "changed content" so it all goes up in one pass
def add_manifest_pak_entry(context, manifest, manifest_path, filesList):  
    manifest_object = {}
    manifest_object['keyName'] = get_standalone_manifest_key(context, manifest_path)
    manifest_object['localFolder'] = dynamic_content_settings.get_pak_folder()
    manifest_object['hash'] = hashlib.md5(open(manifest_path,'rb').read()).hexdigest()
    filesList.append(manifest_object)
    
def _create_manifest_bucket_key_map(context, manifest, manifest_path):
    filesList = _get_files_list(context, manifest, 'Paks')
    
    add_manifest_pak_entry(context, manifest, manifest_path, filesList)
    
    returnMap = {}
    thisFile = {}
    for thisFile in filesList:
        fileKey = _create_bucket_key(thisFile)
        returnMap[fileKey] = thisFile.get('hash','')
    return returnMap

def list_bucket_content(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)
    contentsList = _get_bucket_content_list(context)
    manifestDict = _create_manifest_bucket_key_map(context, manifest, manifest_path)
    thisBucketItem = {}
    for thisBucketItem in contentsList:
        thisKey = thisBucketItem.get('Key')
        if thisKey in manifestDict:
            show_manifest.hash_comparison_bucket(thisKey, manifestDict[thisKey], _get_bucket_item_hash(thisBucketItem))
            del manifestDict[thisKey]
    for remainingKey in manifestDict.keys():
        show_manifest.new_local_key(remainingKey)

def compare_bucket_content(context, args):
    manifest_path, manifest = _get_path_and_manifest(context, args.manifest_path)

    s3 = context.aws.client('s3')
    bucketName = _get_content_bucket(context)
    manifestDict = _create_manifest_bucket_key_map(context, manifest, manifest_path)
    for thisKey, thisHash in manifestDict.iteritems():
        try:
            headResponse = s3.head_object(
                Bucket = bucketName,
                Key = thisKey
            )
        except Exception as e:
            show_manifest.key_not_found(thisKey)
            continue
        show_manifest.hash_comparison_bucket(thisKey, thisHash, _get_bucket_item_hash(headResponse))

def _get_bucket_item_hash(bucketItem):
    return bucketItem.get('Metadata',{}).get(_get_meta_hash_name(),{})

# Retrieve the list of files in the bucket which do not line up with our current manifest
def _get_unmatched_content(context, manifest, manifest_path, deployment_name, do_signing):
    s3 = context.aws.client('s3')
    bucketName = _get_content_bucket_by_name(context, deployment_name)
    manifestDict = _create_manifest_bucket_key_map(context, manifest, manifest_path)
    returnDict = {}
    for thisKey, thisHash in manifestDict.iteritems():
        try:
            headResponse = s3.head_object(
                Bucket = bucketName,
                Key = thisKey
            )
        except Exception as e:
            show_manifest.key_not_found(thisKey)
            returnDict[thisKey] = thisHash
            continue
        show_manifest.hash_comparison_bucket(thisKey, thisHash, _get_bucket_item_hash(headResponse))
        if _get_bucket_item_hash(headResponse) != thisHash or staging.signing_status_changed(context, thisKey, do_signing):
            returnDict[thisKey] = thisHash
    return returnDict

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
    upload_manifest_content(context, args.manifest_path, args.deployment_name, staging_args, args.all, args.signing)

# 1 - Build new paks to ensure our paks are up to date and our manifest reflects the latest changes
# 2 - Update our manifest hashes to match our current content
# 3 - Check each item in our manifest against a HEAD call to get the metadata with our saved local hash values
# 4 - Upload each unmatched pak file
# 5 - Upload the manifest
def upload_manifest_content(context, manifest_path, deployment_name, staging_args, upload_all = False, do_signing = False):
    build_new_paks(context, manifest_path, upload_all)
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    _update_file_hashes(context, manifest_path, manifest)
    remainingContent = _get_unmatched_content(context, manifest, manifest_path, deployment_name, do_signing)
    bucketName = _get_content_bucket_by_name(context, deployment_name)
    filesList = _get_files_list(context, manifest, 'Paks')
    thisFile = {}
    base_content_path = os.path.join(context.config.root_directory_path, context.config.game_directory_name)
    did_upload = False
    uploaded_files = []
    uploaded_signatures = {}
    for thisFile in filesList:
        thisKey = _create_bucket_key(thisFile)
        if thisKey in remainingContent or upload_all:
            this_file_path = _get_path_for_file_entry(context, thisFile, context.config.game_directory_name)
            if thisFile['hash'] == '':
                show_manifest.skipping_invalid_file(this_file_path)
                continue
            show_manifest.found_updated_item(thisKey)
            context.view.found_updated_item(thisKey)
            _do_file_upload(context, this_file_path, bucketName, thisKey, thisFile['hash'])
            did_upload = True
            uploaded_files.append(thisKey)
            
            if do_signing:
                this_signature = signing.get_file_signature(context, _get_path_for_file_entry(context, thisFile, context.config.game_directory_name))
                uploaded_signatures[thisKey] = this_signature
    

    parent_key = get_standalone_manifest_key(context, manifest_path)

    
    if staging_args != None:
        for thisFile in uploaded_files:
            staging_args['Signature'] = uploaded_signatures.get(thisFile)

            

            if parent_key == thisFile:

                staging_args['Parent'] = ''

            else:

                staging_args['Parent'] = parent_key

            

            staging.set_staging_status(thisFile, context, staging_args, deployment_name)

def _upload_manifest(context, manifest_pak_path, deployment_name):
    sourcePath, manifest_name = os.path.split(manifest_pak_path)
    _do_file_upload(context, manifest_pak_path, _get_content_bucket_by_name(context, deployment_name), manifest_name, '')
    return manifest_name
    
def _get_meta_hash_name():
    return 'hash'

def _do_file_upload(context, this_file_path, bucketName, thisKey, hashValue):
    s3 = context.aws.client('s3')
    config = TransferConfig(
        max_concurrency=10,
        num_download_attempts=10,
    )
    transfer = S3Transfer(s3, config)
    transfer.upload_file(this_file_path, bucketName, thisKey,
                            callback=ProgressPercentage(context, this_file_path), extra_args= {'Metadata' : { _get_meta_hash_name() : hashValue}})
    show_manifest.done_uploading(this_file_path)
    context.view.done_uploading(this_file_path)

def _do_put_upload(context, this_file_path, bucketName, thisKey, hashValue):
    s3 = context.aws.client('s3')
    s3.upload_file(this_file_path, bucketName, thisKey, ExtraArgs= {'Metadata' : { _get_meta_hash_name() : hashValue}})

def _get_cache_game_path(context, game_directory):
    return os.path.join('cache', game_directory)

def _get_cache_platform_game_path(context, platform, game_directory):
    return_path = _get_cache_game_path(context, game_directory)
    return_path = os.path.join(return_path, _get_path_for_platform(platform), game_directory)
    return_path = make_end_in_slash(return_path)
    return return_path

def _get_path_for_platform(aliasName):
    if aliasName == None or aliasName == '' or aliasName == 'shared':
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
    platform_cache_game_path =  _get_cache_platform_game_path(context, platform, game_directory)

    full_platform_cache_game_path = os.path.join(context.config.root_directory_path, platform_cache_game_path)
    if not os.path.isdir(full_platform_cache_game_path):
        full_platform_cache_game_path = os.path.join(context.config.root_directory_path, cache_game_path)
    return full_platform_cache_game_path

def _get_game_root_for_file_entry(context, manifestEntry, game_directory):
    return_path = context.config.root_directory_path
    if manifestEntry.get('cacheRoot',{}) == '@assets@':
        return os.path.join(return_path,
                                   _get_cache_platform_game_path(context,
                                                                 manifestEntry.get('platformType', None),
                                                                 game_directory))
    return os.path.join(return_path, game_directory)
    
def _get_root_for_file_entry(context, manifestEntry, game_directory):
    
    return_path = context.config.root_directory_path
    if manifestEntry.get('cacheRoot',{}) == '@assets@':
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
    return_path = os.path.join(_get_root_for_file_entry(context, manifestEntry, game_directory),
                               manifestEntry['keyName']).replace('\\','/')
    return return_path

# Returns the relative pak path for a given entry such as /DynamicContent/Paks/carassets.shared.pak
def _get_pak_for_entry(context, fileEntry, manifest_path):
    pak_path = fileEntry.get('pakFile')

    if not pak_path:
        return None

    return pak_path.replace('\\','/')

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
            #We know this pak needs an update already
            continue

        hex_return = hashlib.md5(open(this_file_path, 'rb').read()).hexdigest()
        manifestHash = thisFile.get('hash', '')
        context.view.hash_comparison_disk(this_file_path, manifestHash, hex_return)

        if hex_return != manifestHash:
            updated_paks.add(relative_pak_folder)

    # pak_all is just a simple way to say give me back all of the data in the expected pak_file, list of files format
    # regardless of updates.  In most cases we want to strip out the paks we haven't found any updates for
    if not pak_all:
        for this_pak in returnData.keys():
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
    return_path = os.path.join(dynamic_content_settings.get_pak_game_folder(context.config.game_directory_path),file_name)    
    return return_path

def create_standalone_manifest_pak(context, manifest_path):
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    _update_file_hashes(context, manifest_path, manifest)
    full_path = os.path.join(context.config.root_directory_path,manifest_path).replace('\\','/')
    archiver = pak_files.PakFileArchiver()
    pak_folder_path = _get_path_for_standalone_manifest_pak(context, full_path)
    files_to_pak = []
    base_content_path = os.path.join(context.config.root_directory_path, context.config.game_directory_name).replace('\\','/')
    file_pair = (full_path, base_content_path)
    files_to_pak.append(file_pair)
    archiver.archive_files(files_to_pak, pak_folder_path)

def get_path_for_manifest_platform(context, manifest_path, platform):
    manifest_root, manifest_name = os.path.split(manifest_path)
    if len(manifest_name):
        manifest_name = manifest_name.split('.json',1)[0]
    else:
        manifest_name = manifest_path
    
    pak_folder_path = os.path.join(dynamic_content_settings.get_pak_folder(), manifest_name + '.' + platform + '.pak')
    if len(pak_folder_path) and pak_folder_path[0]=='/':
        pak_folder_path = pak_folder_path[1:] 
    return pak_folder_path

def get_pak_game_folder(context, relative_folder_path):
	pak_folder_path = os.path.join(context.config.game_directory_path, relative_folder_path).replace('\\','/')
	return pak_folder_path
	
def command_build_new_paks(context, args):
    build_new_paks(context, args.manifest_path, args.all)

def build_new_paks(context, manifest_path, pak_All = False):
#todo add variable for a pak name, and add the pak name to the file entry in the manifest
    manifest_path, manifest = _get_path_and_manifest(context, manifest_path)
    updatedContent = _get_updated_local_content(context, manifest_path, manifest, pak_All)
    for relative_folder_path, files in updatedContent.iteritems():
        # currently this command is only supported on windows
        archiver = pak_files.PakFileArchiver()
        files_to_pak = []
        pak_folder_path = get_pak_game_folder(context, relative_folder_path)
        for file in files:
            file['pakFile'] = relative_folder_path
            file_pair = (_get_path_for_file_entry(context, file, context.config.game_directory_name),
            
                             _get_game_root_for_file_entry(context, file, context.config.game_directory_name))
            files_to_pak.append(file_pair)
            print file_pair
        archiver.archive_files(files_to_pak, pak_folder_path)
    _save_content_manifest(context, manifest_path, manifest)
    
    create_standalone_manifest_pak(context, manifest_path)  

def get_list_objects_limit():
    return 1000 # This is an AWS internal limit on list_objects