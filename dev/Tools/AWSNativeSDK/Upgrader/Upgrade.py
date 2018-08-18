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

import sys
import urllib2
import zipfile
import os
import shutil
import argparse
import re

def main():

    parser = _create_parser()
    args = parser.parse_args()

    version = args.version

    version_url = get_source_url() +  version + '/'

    target_dir = args.destination or build_version_folder(version)

    if not target_dir:
        print 'Missing destination folder'
        return

    print 'Getting items- Services: {} Platforms: {} for version {} from {} to {}'.format(args.service or 'all', args.platform or 'all',version, version_url,target_dir)
    get_all_items(version_url, target_dir, args)

    print 'Finished'
    return

def _create_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('-version', required=True, help='The nativeSDK version to install - e.g. 1.0.74')
    parser.add_argument('-destination', help='The destination folder to install to.  If not supplied the script will look for 3rdParty and attempt to find the correct destination - e.g. F:/3rdParty/AWS/AWSNativeSDK/1.0.74')
    parser.add_argument('-service', help='A name to match against services and install only services which match.  If not supplied will install the default Lumberyard services.')
    parser.add_argument('-platform', help='A name to match against possible platforms and install only platforms which match.  If not supplied will install all Lumberyard supported platforms.')
    parser.add_argument('-extension', help='An extension type to specify for unpacking (e.g. .pdb)')
    parser.add_argument('-noclean', action='store_true',help='Do not delete downloaded zips after operation is complete')
    parser.add_argument('-nofetch', action='store_true',help='Do not download a zip (Must already be present in the download directory)')
    parser.add_argument('-dependencies', action='store_true',help='Skip fetching and unpacking, only install dependencies')
    parser.add_argument('-noheaders', action='store_true',help='Skip installing headers')
    parser.add_argument('-skiprestricted', action='store_true',help='Skip restricted platforms')

    return parser

def get_third_party_name():
    return '3rdParty'

def find_third_party(dirname):
    if not dirname:
        return None

    for curdir, dirnames, filenames in os.walk(dirname):
        if get_third_party_name() in dirnames:
            return os.path.join(curdir, get_third_party_name())
        break
    basepath, filename = os.path.split(dirname)
    if not basepath or basepath == dirname:
        return None
    return find_third_party(basepath)

def get_nativesdk_root():
    thirdparty_folder = find_third_party(os.getcwd())

    if not thirdparty_folder:
        print 'Could not find 3rd party!'
        return None

    return os.path.join(thirdparty_folder, 'AWS', 'AWSNativeSDK')

def build_version_folder(version_string):
    sdk_root = get_nativesdk_root()

    if not sdk_root:
        return None

    return os.path.join(sdk_root, version_string)

def get_default_library_list():
    return_list = []

    return_list.append('cognito-identity')
    return_list.append('cognito-idp')
    return_list.append('core')
    return_list.append('dynamodb')
    return_list.append('gamelift')
    return_list.append('identity-management')
    return_list.append('kinesis')
    return_list.append('lambda')
    return_list.append('mobileanalytics')
    return_list.append('queues')
    return_list.append('s3')
    return_list.append('sns')
    return_list.append('sqs')
    return_list.append('sts')

    return return_list

def get_source_url():
    return 'https://s3.amazonaws.com/aws-sdk-cpp-builds-sdks-team/cpp/builds/'

def get_windows_library_list():
    return_list = get_default_library_list()

    ## Libraries by customer request
    return_list.append('access-management')
    return_list.append('transfer')

    return return_list

def get_windows_vs2015():
    vs2015 = {}
    vs2015['platform'] = 'windows'
    vs2015['zipfile'] = 'aws-sdk-cpp_x86_64_visual_cpp-14x.zip'
    vs2015['libraries'] = get_windows_library_list()
    vs2015['libextensions'] = ['.dll', '.lib']
    vs2015['name'] = 'vs2015'

    return vs2015


def get_windows_vs2013():
    vs2013 = {}
    vs2013['platform'] = 'windows'
    vs2013['zipfile'] = 'aws-sdk-cpp_x86_64_visual_cpp-12x.zip'
    vs2013['libraries'] = get_windows_library_list()
    vs2013['libextensions'] = ['.dll', '.lib']
    vs2013['name'] = 'vs2013'

    return vs2013

def get_darwin():
    darwin_clang = {}
    darwin_clang['platform'] = 'mac'
    darwin_clang['zipfile'] = 'aws-sdk-cpp_x86_64-apple-darwin_clang-3x.zip'
    darwin_clang['libraries'] = get_default_library_list()
    darwin_clang['libextensions'] = ['.dylib', '.a']
    darwin_clang['build_lib_folder'] = True
    darwin_clang['name'] = 'mac'

    return darwin_clang

def get_android_v7_19():
    android_arm = {}
    android_arm['platform'] = 'android'
    android_arm['zipfile'] = 'aws-sdk-cpp-Android-Arm.zip'
    android_arm['libraries'] = get_default_library_list()
    android_arm['libextensions'] = ['.so', '.a']
    android_arm['filter-lib-include'] = 'armeabi-v7a-api-19'
    android_arm['custom_path'] = os.path.join('ndk_r12','android-19','armeabi-v7a','clang-3.8')
    android_arm['build_lib_folder'] = True
    android_arm['name'] = 'androidv7-19'

    return android_arm

def get_android_v7_21():
    android_arm = {}
    android_arm['platform'] = 'android'
    android_arm['zipfile'] = 'aws-sdk-cpp-Android-Arm.zip'
    android_arm['libraries'] = get_default_library_list()
    android_arm['libextensions'] = ['.so', '.a']
    android_arm['filter-lib-include'] = 'armeabi-v7a-api-21'
    android_arm['custom_path'] = os.path.join('ndk_r12','android-21','armeabi-v7a','clang-3.8')
    android_arm['build_lib_folder'] = True
    android_arm['name'] = 'androidv7-21'

    return android_arm

def get_android_v8():
    android_arm = {}
    android_arm['platform'] = 'android'
    android_arm['zipfile'] = 'aws-sdk-cpp-Android-Arm.zip'
    android_arm['libraries'] = get_default_library_list()
    android_arm['libextensions'] = ['.so', '.a']
    android_arm['filter-lib-include'] = 'arm64-v8a-api-21'
    android_arm['custom_path'] = os.path.join('ndk_r12','android-21','arm64-v8a','clang-3.8')
    android_arm['build_lib_folder'] = True
    android_arm['name'] = 'androidv8'

    return android_arm

def get_linux():
    linux_clang = {}
    linux_clang['platform'] = 'linux'
    linux_clang['zipfile'] = 'aws-sdk-cpp_Linux_x86_64_clang-3x.zip'
    linux_clang['libraries'] = get_default_library_list()
    linux_clang['libextensions'] = ['.so', '.a']
    linux_clang['build_lib_folder'] = False # Use the folder structure as is
    linux_clang['name'] = 'linux'

    return linux_clang

def get_appletv():
    appletv_arm64 = {}
    appletv_arm64['platform'] = 'appletv'
    appletv_arm64['zipfile'] = 'aws-sdk-cpp_appletv_arm64_clang-3x.zip'
    appletv_arm64['libraries'] = get_default_library_list()
    appletv_arm64['libextensions'] = ['.a'] # No dylibs on appletv (monolithic only)
    appletv_arm64['build_lib_folder'] = True
    appletv_arm64['name'] = 'appletv'

    return appletv_arm64

def get_ios():
    ios_arm64 = {}
    ios_arm64['platform'] = 'ios'
    ios_arm64['zipfile'] = 'aws-sdk-cpp_iphone_arm64_clang-3x.zip'
    ios_arm64['libraries'] = get_default_library_list()
    ios_arm64['libextensions'] = ['.a'] # No Dylibs on IOS
    ios_arm64['build_lib_folder'] = True
    ios_arm64['name'] = 'ios'

    return ios_arm64

def get_platform_list(args):
    platform_list = []

    platform_list.append(get_windows_vs2015())
    platform_list.append(get_darwin())
    platform_list.append(get_ios())
    platform_list.append(get_android_v7_19())
    platform_list.append(get_android_v7_21())
    platform_list.append(get_android_v8())
    platform_list.append(get_linux())

    if not args.skiprestricted:
        add_restricted_platform_list(platform_list)

    if args.platform is not None:
        platform_list = [ this_obj for this_obj in platform_list if re.search(args.platform, this_obj.get('name'), re.I) ]

    return platform_list

def get_lib_prefix():
    return 'aws-cpp-sdk-'

def fetch_item(platform_info, version_url, args):
    if args.nofetch or platform_info.get('nofetch'):
        print 'Skipping fetch'
        return True
    if args.dependencies:
        print 'Skipping fetch - dependencies only'
        return False
    file_name = platform_info.get('zipfile')
    file_url = version_url + platform_info.get('zipfile')
    print 'fetching {}'.format(file_url)
    file_size = 0
    try:
        result = urllib2.urlopen(file_url)
        file_size = result.headers['content-length']
    except urllib2.HTTPError, errorInfo:
        print 'FAILED - Error code {}'.format(errorInfo.code)
        return False

    read_block_size = 1024 * 1024 * 16

    total_read = 0
    try:
        with open(file_name, "wb") as writehandle:
            while True:
                this_block = result.read(read_block_size)
                print 'Read {} bytes {} / {}'.format(len(this_block), total_read, file_size)
                if not this_block:
                    break
                writehandle.write(this_block)
                total_read += len(this_block)
    except:
        print 'Could not write to {}'.format(file_name)
        return False

    print 'Got {} bytes total'.format(total_read)
    return True

def get_output_paths(platform_info, file_name):
    if not platform_info.get('build_lib_folder'):
        return [ file_name ]
    full_path = file_name.replace('\\','/')
    bin_lib_str = 'bin' if '/bin/' in full_path or full_path.startswith('bin/') else 'lib'
    return_dir, name_and_ext = os.path.split(file_name)
    path_base, debug_release = os.path.split(return_dir)
    custom_path = platform_info.get('custom_path','')
    if debug_release == 'Debug' or debug_release == 'Release':
        return [ os.path.join(bin_lib_str, platform_info.get('platform'), custom_path, debug_release, name_and_ext) ]

    print 'WARNING - Could not determine path for {} - Returning as Debug and Release'.format(file_name)

    return_strings = []
    return_strings.append(os.path.join(bin_lib_str, platform_info.get('platform'),'Debug', name_and_ext))
    return_strings.append(os.path.join(bin_lib_str, platform_info.get('platform'),'Release', name_and_ext))

    return return_strings

def sanitize_file(file_name):
    file_name = file_name.replace('\\','/')
    return file_name

# Determine whether this is a file which should be extracted and where it should go
# Headers come out "as is"
# Libs occasionally need some changes so we use a copy to a specified target rather
# than just extract as is
def get_file_destinations(platform_info, file_name,args):
    library_list = [args.service] if args.service else platform_info.get('libraries')
    extension_list = [args.extension] if args.extension else platform_info['libextensions']
    for this_lib in library_list:
        if (not args.extension or args.extension == '.h') and file_name.startswith('include/aws/{}/'.format(this_lib)):
            if args.noheaders:
                return False, None
            return True, [ file_name ]
        filter_in = platform_info.get('filter-lib-include')
        if filter_in is not None:
            if not filter_in in file_name:
                return False, None
        for libextension in extension_list:
            if file_name.endswith('{}{}{}'.format(get_lib_prefix(), this_lib, libextension)):
                output_strs = get_output_paths(platform_info, file_name)
                return True, output_strs
    return False, None

def is_zip_folder(zip_file_entry):
    if not zip_file_entry:
        return False
    if isinstance(zip_file_entry, basestring):
        return zip_file_entry.endswith('/') or zip_file_entry.endswith('\\')
    return False


def extract_item(platform_info, target_dir, args):
    file_name = platform_info.get('zipfile')   
    read_zip = zipfile.ZipFile(file_name, 'r')

    print '\nExtracting {} to {}\n'.format(file_name, target_dir)

    for zip_file in read_zip.namelist():
        sanitized_file = sanitize_file(zip_file)
        should_unzip, file_paths = get_file_destinations(platform_info, sanitized_file, args)
        if should_unzip:
            for file_path in file_paths:
                file_path = os.path.join(target_dir, file_path)
                archived_file = read_zip.open(zip_file)
                if not os.path.exists(os.path.dirname(file_path)):
                    print 'Creating Path {}\n    -->{}'.format(zip_file, file_path)
                    os.makedirs(os.path.dirname(file_path))
                if not is_zip_folder(file_path):
                    print 'Extracting {}\n    -->{}'.format(zip_file, file_path)
                    close_output = False
                    try:
                        output_file = file(file_path, 'wb')
                        close_output = True
                        shutil.copyfileobj(archived_file, output_file)
                    except IOError as e:
                        print 'Could not copy {} to {} ({})'.format(zip_file, file_path, e)
                    archived_file.close()
                    if close_output:
                        output_file.close()

    read_zip.close()

def cleanup_item(file_name):
    try:
        os.remove(file_name)
        print 'Removed {}'.format(file_name)
    except Exception as e:
        print 'Could not cleanup {} ({})'.format(file_name, e)

def copy_dir(source_dir, dest_dir):
    if not os.path.isdir(dest_dir):
        os.mkdir(dest_dir)
    filenames = os.listdir(source_dir)
    for filename in filenames:
        sourcefile = os.path.join(source_dir, filename)
        destfile = os.path.join(dest_dir, filename)
        if os.path.isfile(sourcefile):
            try:
                print 'Attempting to copy {} to {}'.format(sourcefile, destfile)
                shutil.copy(sourcefile,destfile)
            except IOError as e:
                print 'Could not copy {} to {} ({})'.format(sourcefile, destfile, e)
        elif os.path.isdir(sourcefile):
            copy_dir(sourcefile, destfile)

def copy_extras(platform_info, target_dir):
    source_path = os.path.join(os.getcwd(), platform_info.get('platform'))
    if os.path.exists(source_path):
        copy_dir(source_path, os.path.join(target_dir,'lib', platform_info.get('platform')))

def get_all_items(version_url, target_dir, args):
    file_list = get_platform_list(args) 
    fetched_set = set()

    for thisEntry in file_list:
        file_name = thisEntry.get('zipfile')
        if fetch_item(thisEntry, version_url, args) or file_name in fetched_set:
            extract_item(thisEntry, target_dir, args)
            fetched_set.add(file_name)
            copy_extras(thisEntry, target_dir)

    if not args.noclean:
        for file_name in fetched_set:
            cleanup_item(file_name)

def add_restricted_platform_list(platform_list):
    try:
        from restricted_platforms import add_restricted_platforms
        add_restricted_platforms(platform_list, get_default_library_list())
    except ImportError as e:
        print 'No restricted module found - {}'.format(e.message)
        return False
    else:
        return True

main()
