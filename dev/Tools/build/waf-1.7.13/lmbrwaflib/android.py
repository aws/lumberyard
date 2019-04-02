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
#  Android builder
"""
Usage (in wscript):

def options(opt):
    opt.load('android')

def configure(conf):
    conf.load('android')
"""
import imghdr, os, shutil, stat, string, time

import xml.etree.ElementTree as ET

from contextlib import contextmanager
from datetime import datetime
from subprocess import call, check_output, STDOUT

from branch_spec import get_supported_configurations
from cry_utils import append_to_unique_list, get_command_line_limit
from packaging import run_rc_job, generate_shaders_pak
from utils import junction_directory, remove_junction, write_auto_gen_header

from waflib import Context, TaskGen, Build, Utils, Node, Logs, Options
from waflib.Build import POST_LAZY, POST_AT_ONCE
from waflib.Configure import conf
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waflib.TaskGen import feature, before, before_method, after_method, taskgen_method

from waflib.Tools import ccroot
ccroot.USELIB_VARS['android'] = set([ 'AAPT', 'AAPT_RESOURCES', 'AAPT_INCLUDES', 'AAPT_PACKAGE_FLAGS' ])


################################################################
#                     Defaults                                 #
BUILDER_DIR = 'Code/Launcher/AndroidLauncher/ProjectBuilder'
BUILDER_FILES = 'android_builder.json'
ANDROID_LIBRARY_FILES = 'android_libraries.json'

RESOLUTION_MESSAGE = 'Please re-run Setup Assistant with "Compile For Android" enabled and run the configure command again.'

RESOLUTION_SETTINGS = ( 'mdpi', 'hdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi' )

DEFAULT_CONFIG_CHANGES = [
    'keyboard',
    'keyboardHidden',
    'orientation',
    'screenSize',
    'smallestScreenSize',
    'screenLayout',
    'uiMode',
]

ORIENTATION_LANDSCAPE = 1 << 0
ORIENTATION_PORTRAIT = 1 << 1
ORIENTATION_ALL = (ORIENTATION_LANDSCAPE | ORIENTATION_PORTRAIT)

ORIENTATION_FLAG_TO_KEY_MAP  = {
    ORIENTATION_LANDSCAPE : 'land',
    ORIENTATION_PORTRAIT : 'port',
}

LATEST_KEYWORD = 'latest'

ANDROID_CACHE_FOLDER = 'AndroidCache'

APK_WITH_ASSETS_SUFFIX = '_w_assets'

# these are the default names for application icons and splash images
APP_ICON_NAME = 'app_icon.png'
APP_SPLASH_NAME = 'app_splash.png'

# master list of all supported APIs
SUPPORTED_APIS = [
    'android-19',
    'android-21',
    'android-22',
    'android-23',
    'android-24',
    'android-25',
    'android-26',
    'android-27',
    'android-28',
]

MIN_NDK_REV = 15
MIN_ARMv8_API = 'android-21'
SUPPORTED_NDK_PLATFORMS = sorted(SUPPORTED_APIS)

# changes to the app submission process requires apps to target new APIs for the SDK (Java) version
MIN_SDK_VERSION = 'android-26'
SUPPORTED_SDK_VERSIONS = sorted([ api for api in SUPPORTED_APIS if api >= MIN_SDK_VERSION ])

# keep the minimum build tools version in sync with the min supported SDK version
MIN_BUILD_TOOLS_VERSION = '{}.0.0'.format(MIN_SDK_VERSION.split('-')[1])

# known build tools versions with stability issues
UNSUPPORTED_BUILD_TOOLS_VERSIONS = {
    'win32' : [
    ],
    'darwin' : [
    ]
}

# build tools versions marked as obsolete by the Android SDK manager
OBSOLETE_BUILD_TOOLS_VERSIONS = [
]

# 'defines' for the different asset deployment modes
ASSET_DEPLOY_LOOSE = 'loose'
ASSET_DEPLOY_PAKS = 'paks'
ASSET_DEPLOY_PROJECT_SETTINGS = 'project_settings'

ASSET_DEPLOY_MODES = [
    ASSET_DEPLOY_LOOSE,
    ASSET_DEPLOY_PAKS,
    ASSET_DEPLOY_PROJECT_SETTINGS
]

# root types
ACCESS_NORMAL = 0 # the device is not rooted, we do not have access to any elevated permissions
ACCESS_ROOT_ADBD = 1 # the device is rooted, we have elevated permissions at the adb level
ACCESS_SHELL_SU = 2 # the device is rooted, we only have elevated permissions using 'adb shell su -c'

# The default permissions for installed libraries on device.
LIB_FILE_PERMISSIONS = '755'

# The default owner:group for installed libraries on device.
LIB_OWNER_GROUP = 'system:system'

ADB_QUOTED_PATH = None

#                                                              #
################################################################

@contextmanager
def push_dir(directory):
    """
    Temporarily changes the current working directory.  By decorating it with the contexmanager, makes this function only
    usable in "with" statements, otherwise its a no-op.  When the "with" statement is executed, this function will run
    till the yield, then run what's inside the "with" statement and finally run what's after the yield.
    """
    previous_dir = os.getcwd()
    os.chdir(directory)
    yield
    os.chdir(previous_dir)


################################################################
@feature('cshlib', 'cxxshlib')
@after_method('apply_link')
def apply_so_name(self):
    """
    Adds the linker flag to set the DT_SONAME in ELF shared objects. The
    name used here will be used instead of the file name when the dynamic
    linker attempts to load the shared object
    """

    if 'android' in self.bld.env['PLATFORM'] and self.env.SONAME_ST:
        flag = self.env.SONAME_ST % self.link_task.outputs[0]
        self.env.append_value('LINKFLAGS', flag.split())


################################################################
@conf
def get_android_api_lib_list(ctx):
    """
    Gets a list of android apis that pre-built libs could be built against based
    on the current build target e.g. NDK_PLATFORM
    """
    ndk_platform = ctx.get_android_ndk_platform()
    try:
        index = SUPPORTED_NDK_PLATFORMS.index(ndk_platform)
    except:
        ctx.fatal('[ERROR] Unsupported Android NDK platform version %s', ndk_platform)
    else:
        # we can only use libs built with api levels lower or equal to the one being used
        return SUPPORTED_NDK_PLATFORMS[:index + 1] # end index is exclusive, so we add 1


################################################################
@conf
def is_android_armv8_api_valid(ctx):
    """
    Checks to make sure desired API level meets the min spec for ARMv8 targets
    """
    ndk_platform = ctx.get_android_ndk_platform()
    return (ndk_platform >= MIN_ARMv8_API)


################################################################
def remove_file_and_empty_directory(directory, file_name):
    """
    Helper function for deleting a file and directory, if empty
    """

    file_path = os.path.join(directory, file_name)

    # first delete the file, if it exists
    if os.path.exists(file_path):
        os.remove(file_path)

    # then remove the directory, if it exists and is empty
    if os.path.exists(directory) and not os.listdir(directory):
        os.rmdir(directory)


################################################################
def remove_readonly(func, path, _):
    '''Clear the readonly bit and reattempt the removal'''
    os.chmod(path, stat.S_IWRITE)
    func(path)


################################################################
def construct_source_path(conf, project, source_path):
    """
    Helper to construct the source path to an asset override such as
    application icons or splash screen images
    """
    if os.path.isabs(source_path):
        path_node = conf.root.make_node(source_path)
    else:
        path_node = conf.root.make_node([ Context.launch_dir, conf.game_code_folder(project), 'Resources', source_path ])
    return path_node.abspath()


################################################################
def clear_splash_assets(project_node, path_prefix):

    target_directory = project_node.make_node(path_prefix)
    remove_file_and_empty_directory(target_directory.abspath(), APP_SPLASH_NAME)

    for resolution in RESOLUTION_SETTINGS:
        # The xxxhdpi resolution is only for application icons, its overkill to include them for drawables... for now
        if resolution == 'xxxhdpi':
            continue

        target_directory = project_node.make_node(path_prefix + '-' + resolution)
        remove_file_and_empty_directory(target_directory.abspath(), APP_SPLASH_NAME)


################################################################
def options(opt):
    group = opt.add_option_group('android-specific config')

    group.add_option('--android-sdk-version-override', dest = 'android_sdk_version_override', action = 'store', default = None, help = 'Override the Android SDK version used in the Java compilation.  Only works during configure.')
    group.add_option('--android-ndk-platform-override', dest = 'android_ndk_platform_override', action = 'store', default = None, help = 'Override the Android NDK platform version used in the native compilation.  Only works during configure.')

    group.add_option('--dev-store-pass', dest = 'dev_store_pass', action = 'store', default = 'Lumberyard', help = 'The store password for the development keystore')
    group.add_option('--dev-key-pass', dest = 'dev_key_pass', action = 'store', default = 'Lumberyard', help = 'The key password for the development keystore')

    group.add_option('--distro-store-pass', dest = 'distro_store_pass', action = 'store', default = '', help = 'The store password for the distribution keystore')
    group.add_option('--distro-key-pass', dest = 'distro_key_pass', action = 'store', default = '', help = 'The key password for the distribution keystore')

    group.add_option('--from-editor-deploy', dest = 'from_editor_deploy', action = 'store_true', default = False, help = 'Signals that the build is coming from the editor deployment tool')

    group.add_option('--deploy-android-attempt-libs-only', dest = 'deploy_android_attempt_libs_only', action = 'store_true', default = False,
        help = 'Will only push the changed native libraries.  If "deploy_android_executable" is enabled, it will take precedent if modified.  Option ignored if "deploy_android_clean_device" is enabled.  This feature is only available for "unlocked" devices.')


################################################################
def configure(conf):

    def _lst_to_str(lst):
        return ', '.join(lst)

    Logs.info('Validating Android SDK/NDK installation...')

    env = conf.env

    # validate the stored sdk and ndk paths from SetupAssistant
    sdk_root = conf.get_env_file_var('LY_ANDROID_SDK', required = True)
    ndk_root = conf.get_env_file_var('LY_ANDROID_NDK', required = True)

    if not (sdk_root and ndk_root):
        missing_paths = []
        missing_paths += ['Android SDK'] if not sdk_root else []
        missing_paths += ['Android NDK'] if not ndk_root else []

        conf.fatal('[ERROR] Missing paths from Setup Assistant detected for: {}.  {}'.format(_lst_to_str(missing_paths), RESOLUTION_MESSAGE))

    env['ANDROID_SDK_HOME'] = sdk_root
    env['ANDROID_NDK_HOME'] = ndk_root

    # get the revision of the NDK
    ndk_rev = None

    properties_file_path = os.path.join(ndk_root, 'source.properties')
    if os.path.exists(properties_file_path):
        with open(properties_file_path) as ndk_props_file:
            for line in ndk_props_file.readlines():
                tokens = line.split('=')
                trimed_tokens = [token.strip() for token in tokens]

                if 'Pkg.Revision' in trimed_tokens:
                    ndk_rev = trimed_tokens[1]

    if not ndk_rev:
        conf.fatal('[ERROR] Unable to validate Android NDK version in path {}.  Please confirm the path to the Android NDK '
            'in Setup Assistant is pointing NDK r{} or higher and run the configure command again'.format(ndk_root, MIN_NDK_REV))

    ndk_rev_tokens = ndk_rev.split('.')
    ndk_rev_major = int(ndk_rev_tokens[0])
    ndk_rev_minor = int(ndk_rev_tokens[1])
    ndk_rev_suffix = '' if not ndk_rev_minor else chr(ord('a') + ndk_rev_minor)

    if ndk_rev_major < MIN_NDK_REV:
        conf.fatal('[ERROR] The version of the Android NDK in use - r{}{} - is below the minimum supported version - r{}.  '
                    'Please select a newer version of the Android NDK in Setup Assistant and run the configure command again.'.format(ndk_rev_major, ndk_rev_suffix, MIN_NDK_REV))

    env['ANDROID_NDK_REV_FULL'] = ndk_rev
    env['ANDROID_NDK_REV_MAJOR'] = ndk_rev_major
    env['ANDROID_NDK_REV_MINOR'] = ndk_rev_minor

    # validate the desired SDK (Java) version
    installed_sdk_versions = os.listdir(os.path.join(sdk_root, 'platforms'))
    valid_sdk_versions = sorted([ platform for platform in installed_sdk_versions if platform in SUPPORTED_SDK_VERSIONS ])
    Logs.debug('android: Valid SDK versions installed are: {}'.format(valid_sdk_versions))

    if not valid_sdk_versions:
        conf.fatal('[ERROR] Unable to detect a valid Android SDK version installed in path {}.  '
                    'Please use the Android SDK Manager to download an appropriate SDK version and run the configure command again.\n'
                    '\t-> Supported versions are: {}'.format(sdk_root, _lst_to_str(SUPPORTED_SDK_VERSIONS)))

    sdk_version = Options.options.android_sdk_version_override or conf.get_android_sdk_version()
    if not sdk_version:
        conf.fatal('[ERROR] No Android SDK version specified!  Please confirm the "SDK_VERSION" entry is set in '
                    '_WAF_/android/android_setting.json and run the configure command again.\n'
                    '\t-> Supported versions installed are: {}'.format(_lst_to_str(valid_sdk_versions)))

    elif sdk_version.lower() == LATEST_KEYWORD:
        sdk_version = valid_sdk_versions[-1]
        Logs.debug('android: Using the latest installed Android SDK version {}'.format(sdk_version))

    elif sdk_version in SUPPORTED_APIS:
        if sdk_version not in SUPPORTED_SDK_VERSIONS:
            conf.fatal('[ERROR] Android SDK version - {} - is below the minimum target SDK version required for app '
                        'submission.  Please change "SDK_VERSION" in _WAF_/android/android_setting.json to a supported '
                        'API and run the configure command again.\n'
                        '\t-> Supported versions installed are: {}'.format(sdk_version, _lst_to_str(valid_sdk_versions)))

        elif sdk_version not in valid_sdk_versions:
            conf.fatal('[ERROR] Failed to find Android SDK version - {} - installed in path {}.  Please use the Android '
                        'SDK Manager to download the desired SDK version or change "SDK_VERSION" in _WAF_/android/android_settings.json '
                        'to a supported version installed and run the configure command again.\n'
                        '\t-> Supported versions installed are: {}'.format(sdk_version, sdk_root, _lst_to_str(valid_sdk_versions)))

    else:
        conf.fatal('[ERROR] Android SDK version - {} - is unsupported.  Please change "SDK_VERSION" in '
                    '_WAF_/android/android_setting.json to a supported API and run the configure command again.\n'
                    '\t-> Supported versions installed are: {}'.format(sdk_version, _lst_to_str(valid_sdk_versions)))

    env['ANDROID_SDK_VERSION'] = sdk_version
    env['ANDROID_SDK_VERSION_NUMBER'] = int(sdk_version.split('-')[1])

    # validate the desired NDK platform version
    if ndk_rev_major >= 19:
        platforms_file = conf.root.make_node([ ndk_root, 'meta', 'platforms.json'])

        json_data = conf.parse_json_file(platforms_file)
        if not json_data:
            conf.fatal('[ERROR] Failed to find platforms file in path {}.'.format(platforms_file.abspath()))

        platform_aliases = json_data['aliases']
        installed_ndk_platforms = set(['android-{}'.format(platform_number) for platform_number in platform_aliases.values()])
    else:
        installed_ndk_platforms = os.listdir(os.path.join(ndk_root, 'platforms'))

    valid_ndk_platforms = sorted([ platform for platform in installed_ndk_platforms if platform in SUPPORTED_NDK_PLATFORMS ])
    Logs.debug('android: Valid NDK platforms for revision {} are: {}'.format(ndk_rev, valid_ndk_platforms))

    ndk_platform = Options.options.android_ndk_platform_override or conf.get_android_ndk_platform()
    if not ndk_platform:
        conf.fatal('[ERROR] No Android NDK platform specified!  Please confirm the "NDK_PLATFORM" entry is set in '
                    '_WAF_/android/android_setting.json and run the configure command again.\n'
                    '\t-> Supported platforms for NDK {} are: {}'.format(ndk_rev, _lst_to_str(valid_ndk_platforms)))

    elif ndk_platform in SUPPORTED_NDK_PLATFORMS:
        # search for the closest, lower match in the event the specified native platform API is in the
        # supported list but the version of the NDK used doesn't have pre-built libraries for it
        if ndk_platform not in valid_ndk_platforms:
            best_match = ndk_platform
            for platform in valid_ndk_platforms:
                if platform <= ndk_platform:
                    best_match = platform

            Logs.warn('[WARN] The Android NDK in use - {} - does not directly support the desired platform version - {}.  '
                        'Falling back to closest match found: {}.\nConsider explicitly setting the "NDK_PLATFORM" in '
                        '_WAF_/android/android_setting.json to a valid supported version part of the NDK package.\n'
                        '\t-> Supported platforms are: {}'.format(ndk_rev, ndk_platform, best_match, _lst_to_str(valid_ndk_platforms)))
            ndk_platform = best_match

    else:
        conf.fatal('[ERROR] Android NDK platform - {} - is unsupported.  Please change "NDK_PLATFORM" in '
                    '_WAF_/android/android_setting.json to a supported API and run the configure command again.\n'
                    '\t-> Supported platforms for NDK {} are: {}'.format(ndk_platform, ndk_rev, _lst_to_str(valid_ndk_platforms)))

    env['ANDROID_NDK_PLATFORM'] = ndk_platform
    env['ANDROID_NDK_PLATFORM_NUMBER'] = int(ndk_platform.split('-')[1])

    # final check is to make sure the ndk platform <= sdk version to ensure compatibility
    if not (ndk_platform <= sdk_version):
        conf.fatal('[ERROR] The Android API specified in "NDK_PLATFORM" - {} - is newer than the API specified '
                    'in "SDK_VERSION" - {}; this can lead to compatibility issues.\nPlease update your '
                    '_WAF_/android/android_settings.json to make sure "NDK_PLATFORM" <= "SDK_VERSION" and '
                    'run the configure command again.'.format(ndk_platform, sdk_version))

    # validate the desired SDK build-tools version
    build_tools_dir_contents = os.listdir(os.path.join(sdk_root, 'build-tools'))
    installed_build_tools_versions = [ entry for entry in build_tools_dir_contents if entry.split('.')[0].isdigit() ]

    host_unsupported_build_tools_versions = UNSUPPORTED_BUILD_TOOLS_VERSIONS.get(Utils.unversioned_sys_platform(), [])
    unusable_build_tools_versions = host_unsupported_build_tools_versions + OBSOLETE_BUILD_TOOLS_VERSIONS

    valid_build_tools_versions = sorted([ entry for entry in installed_build_tools_versions if entry >= MIN_BUILD_TOOLS_VERSION and entry not in unusable_build_tools_versions ])
    Logs.debug('android: Valid build-tools versions installed are: {}'.format(valid_build_tools_versions))

    if not valid_build_tools_versions:
        additional_details = ''
        if host_unsupported_build_tools_versions:
            additional_details = 'Also note the following versions are unsupported:\n\t-> {}'.format(_lst_to_str(host_unsupported_build_tools_versions))

        conf.fatal('[ERROR] Unable to detect a valid Android SDK build-tools version installed in path {}.  '
                    'Please use the Android SDK Manager to download build-tools version {} or higher and run '
                    'the configure command again.  {}'.format(sdk_root, MIN_BUILD_TOOLS_VERSION, additional_details))

    build_tools_version = conf.get_android_build_tools_version()
    if not build_tools_version:
        conf.fatal('[ERROR] No Android build-tools version specified!  Please confirm the "BUILD_TOOLS_VER" entry is '
                    'set in _WAF_/android/android_setting.json and run the configure command again.\n'
                    '\t-> Supported versions installed are: {}'.format(_lst_to_str(valid_build_tools_versions)))

    elif build_tools_version.lower() == LATEST_KEYWORD:
        build_tools_version = valid_build_tools_versions[-1]
        Logs.debug('android: Using the latest installed Android SDK build-tools version {}'.format(build_tools_version))

    elif build_tools_version >= MIN_BUILD_TOOLS_VERSION:
        if build_tools_version in OBSOLETE_BUILD_TOOLS_VERSIONS:
            Logs.warn('[WARN] The Android SDK build-tools version - {} - has been marked as obsolete by Google.  '
                        'Consider using a different version of the build tools by changing "BUILD_TOOLS_VER" in '
                        '_WAF_/android/android_settings.json to "latest" or to another valid installed version '
                        'and run the configure command again.\n'
                        '\t-> Supported versions installed are: {}'.format(build_tools_version, _lst_to_str(valid_build_tools_versions)))

        elif build_tools_version not in valid_build_tools_versions:
            conf.fatal('[ERROR] Failed to find Android SDK build-tools version - {} - installed in path {}.  '
                        'Please use the Android SDK Manager to download the appropriate build-tools version '
                        'or change "BUILD_TOOLS_VER" in _WAF_/android/android_settings.json to a supported '
                        'version installed and run the configure command again.\n'
                        '\t-> Supported versions installed are: {}'.format(build_tools_version, sdk_root, _lst_to_str(valid_build_tools_versions)))

    else:
        conf.fatal('[ERROR] Android SDK build-tools version - {} - is unsupported.  Please change "BUILD_TOOLS_VER" in'
                    '_WAF_/android/android_setting.json to {} or higher and run the configure command again.\n'
                    '\t-> Supported versions installed are: {}'.format(build_tools_version, MIN_BUILD_TOOLS_VERSION, _lst_to_str(valid_build_tools_versions)))

    conf.env['ANDROID_BUILD_TOOLS_VER'] = build_tools_version

    Logs.info('Finished validating Android SDK/NDK installation!')


################################################################
@conf
def get_android_cache_node(conf):
    return conf.get_bintemp_folder_node().make_node(ANDROID_CACHE_FOLDER)


################################################################
@conf
def add_to_android_cache(conf, path_to_resource, arch_override = None):
    """
    Adds resource files from outside the engine folder into a local cache directory so they can be used by WAF tasks.
    Returns the path of the new resource file relative the cache root.
    """

    cache_node = get_android_cache_node(conf)
    cache_node.mkdir()

    dest_node = cache_node

    dest_subfolder = arch_override or conf.env['ANDROID_ARCH']
    if dest_subfolder:
        dest_node = cache_node.make_node(dest_subfolder)
        dest_node.mkdir()

    file_name = os.path.basename(path_to_resource)

    files_node = dest_node.make_node(file_name)
    files_node.delete()

    shutil.copy2(path_to_resource, files_node.abspath())
    files_node.chmod(Utils.O755)

    rel_path = files_node.path_from(cache_node)
    Logs.debug('android: Adding resource - {} - to Android cache'.format(rel_path))
    return rel_path


################################################################
def process_json(conf, json_data, curr_node, root_node, template, copied_files):
    for elem in json_data:

        if elem == 'NO_OP':
            continue

        if os.path.isabs(elem):
            source_curr = conf.root.make_node(elem)
        else:
            source_curr = root_node.make_node(elem)

        target_curr = curr_node.make_node(elem)

        if isinstance(json_data, dict):
            # resolve name overrides for the copy, if specified
            if isinstance(json_data[elem], unicode) or isinstance(json_data[elem], str):
                target_curr = curr_node.make_node(json_data[elem])

            # otherwise continue processing the tree
            else:
                target_curr.mkdir()
                process_json(conf, json_data[elem], target_curr, root_node, template, copied_files)
                continue

        # leaf handing
        if imghdr.what(source_curr.abspath()) in ( 'rgb', 'gif', 'pbm', 'ppm', 'tiff', 'rast', 'xbm', 'jpeg', 'bmp', 'png' ):
            shutil.copyfile(source_curr.abspath(), target_curr.abspath())
        else:
            transformed_text = string.Template(source_curr.read()).substitute(template)
            target_curr.write(transformed_text)

        target_curr.chmod(Utils.O755)
        copied_files.append(target_curr.abspath())


################################################################
def copy_and_patch_android_libraries(conf, source_node, android_root):
    """
    Copy the libraries that need to be patched and do the patching of the files.
    """

    class _Library:
        def __init__(self, name, path):
            self.name = name
            self.path = path
            self.patch_files = []

        def add_file_to_patch(self, file):
            self.patch_files.append(file)

    class _File:
        def __init__(self, path):
            self.path = path
            self.changes = []

        def add_change(self, change):
            self.changes.append(change)

    class _Change:
        def __init__(self, line, old, new):
            self.line = line
            self.old = old
            self.new = new

    lib_src_file = source_node.make_node(ANDROID_LIBRARY_FILES)

    json_data = conf.parse_json_file(lib_src_file)
    if not json_data:
        conf.fatal('[ERROR] Android library settings (%s) not found or invalid.' % ANDROID_LIBRARY_FILES)
        return False

    # Collect the libraries that need to be patched
    libs_to_patch = []
    for libName, value in json_data.iteritems():
        # The library is in different places depending on the revision, so we must check multiple paths.
        srcDir = None
        for path in value['srcDir']:
            path = string.Template(path).substitute(conf.env)
            if os.path.exists(path):
                srcDir = path
                break

        if not srcDir:
            conf.fatal('[ERROR] Failed to find library - %s - in path(s) [%s]. Please download the library from the Android SDK Manager and run the configure command again.'
                        % (libName, ", ".join(string.Template(path).substitute(conf.env) for path in value['srcDir'])))
            return False

        if 'patches' in value:
            lib_to_patch = _Library(libName, srcDir)
            for patch in value['patches']:
                file_to_patch =  _File(patch['path'])
                for change in patch['changes']:
                    lineNum = change['line']
                    oldLines = change['old']
                    newLines = change['new']
                    for oldLine in oldLines[:-1]:
                        change = _Change(lineNum, oldLine, (newLines.pop() if newLines else None))
                        file_to_patch.add_change(change)
                        lineNum += 1
                    else:
                        change = _Change(lineNum, oldLines[-1], ('\n'.join(newLines) if newLines else None))
                        file_to_patch.add_change(change)

                lib_to_patch.add_file_to_patch(file_to_patch)
            libs_to_patch.append(lib_to_patch)

    # Patch the libraries
    for lib in libs_to_patch:
        cur_path = conf.path.abspath()
        rel_path = conf.get_android_patched_libraries_relative_path()
        dest_path = os.path.join(cur_path, rel_path, lib.name)
        shutil.rmtree(dest_path, ignore_errors=True, onerror=remove_readonly)
        shutil.copytree(lib.path, dest_path)
        for file in lib.patch_files:
            inputFilePath = os.path.join(lib.path, file.path)
            outputFilePath = os.path.join(dest_path, file.path)
            with open(inputFilePath) as inputFile:
                lines = inputFile.readlines()

            with open(outputFilePath, 'w') as outFile:
                for replace in file.changes:
                    lines[replace.line] = string.replace(lines[replace.line], replace.old, (replace.new if replace.new else ""), 1)

                outFile.write(''.join([line for line in lines if line]))

    return True


################################################################
def process_multi_window_settings(conf, game_project, multi_window_settings, template):

    def _is_number_option_valid(value, name):
        if value:
            if isinstance(value, int):
                return True
            else:
                Logs.warn('[WARN] Invalid value for property "{}", expected whole number'.format(name))
        return False

    launch_in_fullscreen = False

    # the Samsung DEX specific values can be added regardless of target API and multi-window support
    samsung_dex_options = multi_window_settings.get('samsung_dex_options', None)
    if samsung_dex_options:
        launch_in_fullscreen = samsung_dex_options.get('launch_in_fullscreen', False)

        # setting the launch window size in DEX mode since launching in fullscreen is strictly tied
        # to multi-window being enabled
        launch_width = samsung_dex_options.get('launch_width', None)
        launch_height = samsung_dex_options.get('launch_height', None)

        # both have to be specified otherwise they are ignored
        if _is_number_option_valid(launch_width, 'launch_width') and _is_number_option_valid(launch_height, 'launch_height'):
            template['SAMSUNG_DEX_LAUNCH_WIDTH'] = ('<meta-data '
                'android:name="com.samsung.android.sdk.multiwindow.dex.launchwidth" '
                'android:value="{}"/>'.format(launch_width))

            template['SAMSUNG_DEX_LAUNCH_HEIGHT'] = ('<meta-data '
                'android:name="com.samsung.android.sdk.multiwindow.dex.launchheight" '
                'android:value="{}"/>'.format(launch_height))

        keep_alive = samsung_dex_options.get('keep_alive', None)
        if keep_alive in ( True, False ):
            template['SAMSUNG_DEX_KEEP_ALIVE'] = '<meta-data android:name="com.samsung.android.keepalive.density" android:value="{}" />'.format(str(keep_alive).lower())

    sdk_version = conf.get_android_sdk_version()
    multi_window_enabled = multi_window_settings.get('enabled', False)

    # the option to change the display resolution was added in API 24 as well, these changes are sent as density changes
    template['ANDROID_CONFIG_CHANGES'] = '|'.join(DEFAULT_CONFIG_CHANGES + [ 'density' ])

    # if targeting above the min API level the default value for this attribute is true so we need to explicitly disable it
    template['ANDROID_MULTI_WINDOW'] = 'android:resizeableActivity="{}"'.format(str(multi_window_enabled).lower())

    if not multi_window_enabled:
        return False

    # remove the DEX launch window size if requested to launch in fullscreen mode
    if launch_in_fullscreen:
        template['SAMSUNG_DEX_LAUNCH_WIDTH'] = ''
        template['SAMSUNG_DEX_LAUNCH_HEIGHT'] = ''

    default_width = multi_window_settings.get('default_width', None)
    default_height = multi_window_settings.get('default_height', None)

    min_width = multi_window_settings.get('min_width', None)
    min_height = multi_window_settings.get('min_height', None)

    gravity = multi_window_settings.get('gravity', None)

    layout = ''
    if any([ default_width, default_height, min_width, min_height, gravity ]):
        layout = '<layout '

        # the default width/height values are respected as launch values in DEX mode so they should
        # be ignored if the intension is to launch in fullscreen when running in DEX mode
        if not launch_in_fullscreen:
            if _is_number_option_valid(default_width, 'default_width'):
                layout += 'android:defaultWidth="{}dp" '.format(default_width)

            if _is_number_option_valid(default_height, 'default_height'):
                layout += 'android:defaultHeight="{}dp" '.format(default_height)

        if _is_number_option_valid(min_height, 'min_height'):
            layout += 'android:minHeight="{}dp" '.format(min_height)

        if _is_number_option_valid(min_width, 'min_width'):
            layout += 'android:minWidth="{}dp" '.format(min_width)

        if gravity:
            layout += 'android:gravity="{}" '.format(gravity)

        layout += '/>'

    template['ANDROID_MULTI_WINDOW_PROPERTIES'] = layout

    return True


@conf
def create_base_android_projects(conf):
    """
    This function will generate the bare minimum android project
    and include the new android launcher(s) in the build path.
    So no Android Studio gradle files will be generated.
    """
    android_root = conf.path.make_node(conf.get_android_project_relative_path())
    android_root.mkdir()

    if conf.is_engine_local():
        source_node = conf.path.make_node(BUILDER_DIR)
    else:
        source_node = conf.root.make_node(os.path.abspath(os.path.join(conf.engine_path,BUILDER_DIR)))
    builder_file_src = source_node.make_node(BUILDER_FILES)
    builder_file_dest = conf.path.get_bld().make_node(BUILDER_DIR)

    if not os.path.exists(builder_file_src.abspath()):
        conf.fatal('[ERROR] Failed to find the Android project builder - %s - in path %s.  Verify file exists and run the configure command again.' % (BUILDER_FILES, BUILDER_DIR))
        return False

    created_directories = []
    for project in conf.get_enabled_game_project_list():

        # make sure the project has android options specified
        android_settings = conf.get_android_settings(project)
        if not android_settings:
            Logs.warn('[WARN] Android settings not found in %s/project.json, skipping.' % project)
            continue

        proj_root = android_root.make_node(conf.get_executable_name(project))
        proj_root.mkdir()

        created_directories.append(proj_root.path_from(android_root))

        proj_src_path = os.path.join(proj_root.abspath(), 'src')
        if os.path.exists(proj_src_path):
            shutil.rmtree(proj_src_path, ignore_errors=True, onerror=remove_readonly)

        # setup the macro replacement map for the builder files
        activity_name = '%sActivity' % project
        transformed_package = conf.get_android_package_name(project).replace('.', '/')

        template = {
            'ANDROID_PACKAGE' : conf.get_android_package_name(project),
            'ANDROID_PACKAGE_PATH' : transformed_package,

            'ANDROID_APP_NAME' : conf.get_launcher_product_name(project),    # external facing name
            'ANDROID_PROJECT_NAME' : project,                                # internal facing name

            'ANDROID_PROJECT_ACTIVITY' : activity_name,
            'ANDROID_LAUNCHER_NAME' : conf.get_executable_name(project),     # first native library to load from java

            'ANDROID_VERSION_NUMBER' : conf.get_android_version_number(project),
            'ANDROID_VERSION_NAME' : conf.get_android_version_name(project),

            'ANDROID_CONFIG_CHANGES' : '|'.join(DEFAULT_CONFIG_CHANGES),
            'ANDROID_SCREEN_ORIENTATION' : conf.get_android_orientation(project),

            'ANDROID_APP_PUBLIC_KEY' : conf.get_android_app_public_key(project),
            'ANDROID_APP_OBFUSCATOR_SALT' : conf.get_android_app_obfuscator_salt(project),

            'ANDROID_USE_MAIN_OBB' : conf.get_android_use_main_obb(project),
            'ANDROID_USE_PATCH_OBB' : conf.get_android_use_patch_obb(project),
            'ANDROID_ENABLE_KEEP_SCREEN_ON' : conf.get_android_enable_keep_screen_on(project),
            'ANDROID_DISABLE_IMMERSIVE_MODE' : conf.get_android_disable_immersive_mode(project),

            'ANDROID_MIN_SDK_VERSION' : conf.env['ANDROID_NDK_PLATFORM_NUMBER'],
            'ANDROID_TARGET_SDK_VERSION' : conf.env['ANDROID_SDK_VERSION_NUMBER'],

            'ANDROID_MULTI_WINDOW' : '',
            'ANDROID_MULTI_WINDOW_PROPERTIES' : '',

            'SAMSUNG_DEX_KEEP_ALIVE' : '',
            'SAMSUNG_DEX_LAUNCH_WIDTH' : '',
            'SAMSUNG_DEX_LAUNCH_HEIGHT' : '',
        }

        is_multi_window = False
        multi_window_settings = android_settings.get('multi_window_options', None)
        if multi_window_settings:
            is_multi_window = process_multi_window_settings(conf, project, multi_window_settings, template)

        # when multi-window support is enabled, the desired orientation is more of a suggestion
        orientation = ORIENTATION_ALL
        if not is_multi_window:
            requested_orientation = conf.get_android_orientation(project)

            if requested_orientation in ('landscape', 'reverseLandscape', 'sensorLandscape', 'userLandscape'):
                orientation = ORIENTATION_LANDSCAPE

            elif requested_orientation in ('portrait', 'reversePortrait', 'sensorPortrait', 'userPortrait'):
                orientation = ORIENTATION_PORTRAIT

        # update the builder file with the correct package name
        transformed_node = builder_file_dest.find_or_declare('%s_builder.json' % project)

        transformed_text = string.Template(builder_file_src.read()).substitute(template)
        transformed_node.write(transformed_text)

        # process the builder file and create project
        copied_files = []
        json_data = conf.parse_json_file(transformed_node)
        process_json(conf, json_data, proj_root, source_node, template, copied_files)

        # resolve the application icon overrides
        resource_node = proj_root.make_node(['src', 'main', 'res'])

        icon_overrides = conf.get_android_app_icons(project)
        if icon_overrides:
            mipmap_path_prefix = 'mipmap'

            # if a default icon is specified, then copy it into the generic mipmap folder
            default_icon = icon_overrides.get('default', None)

            if default_icon is not None:
                default_icon_source_node = construct_source_path(conf, project, default_icon)

                default_icon_target_dir = resource_node.make_node(mipmap_path_prefix)
                default_icon_target_dir.mkdir()

                dest_file = os.path.join(default_icon_target_dir.abspath(), APP_ICON_NAME)
                shutil.copyfile(default_icon_source_node, dest_file)
                os.chmod(dest_file, Utils.O755)
                copied_files.append(dest_file)

            else:
                Logs.debug('android: No default icon override specified for %s' % project)

            # process each of the resolution overrides
            warnings = []
            for resolution in RESOLUTION_SETTINGS:
                target_directory = resource_node.make_node(mipmap_path_prefix + '-' + resolution)

                # get the current resolution icon override
                icon_source = icon_overrides.get(resolution, default_icon)
                if icon_source is default_icon:

                    # if both the resolution and the default are unspecified, warn the user but do nothing
                    if icon_source is None:
                        warnings.append(
                            'No icon override found for "{}".  Either supply one for "{}" or a "default" in the android_settings "icon" section of the project.json file for {}'.format(resolution, resolution, project)
                        )

                    # if only the resolution is unspecified, remove the resolution specific version from the project
                    else:
                        Logs.debug('android: Default icon being used for "%s" in %s' % (resolution, project))
                        remove_file_and_empty_directory(target_directory.abspath(), APP_ICON_NAME)

                    continue

                icon_source_node = construct_source_path(conf, project, icon_source)
                icon_target_node = target_directory.make_node(APP_ICON_NAME)

                shutil.copyfile(icon_source_node, icon_target_node.abspath())
                icon_target_node.chmod(Utils.O755)
                copied_files.append(icon_target_node.abspath())

            # guard against spamming warnings in the case the icon override block is full of garbage and no actual overrides
            if len(warnings) != len(RESOLUTION_SETTINGS):
                for warning_msg in warnings:
                    Logs.warn('[WARN] {}'.format(warning_msg))

        # resolve the application splash screen overrides
        splash_overrides = conf.get_android_app_splash_screens(project)
        if splash_overrides:
            drawable_path_prefix = 'drawable-'

            for orientation_flag, orientation_key in ORIENTATION_FLAG_TO_KEY_MAP.iteritems():
                orientation_path_prefix = drawable_path_prefix + orientation_key

                oriented_splashes = splash_overrides.get(orientation_key, {})

                unused_override_warning = None
                if (orientation & orientation_flag) == 0:
                    unused_override_warning = 'Splash screen overrides specified for "{}" when desired orientation is set to "{}" in project {}.  These overrides will be ignored.'.format(
                                orientation_key,
                                ORIENTATION_FLAG_TO_KEY_MAP[orientation],
                                project)

                # if a default splash image is specified for this orientation, then copy it into the generic drawable-<orientation> folder
                default_splash_img = oriented_splashes.get('default', None)

                if default_splash_img is not None:
                    if unused_override_warning:
                        Logs.warn('[WARN] {}'.format(unused_override_warning))
                        continue

                    default_splash_img_source_node = construct_source_path(conf, project, default_splash_img)

                    default_splash_img_target_dir = resource_node.make_node(orientation_path_prefix)
                    default_splash_img_target_dir.mkdir()

                    dest_file = os.path.join(default_splash_img_target_dir.abspath(), APP_SPLASH_NAME)
                    shutil.copyfile(default_splash_img_source_node, dest_file)
                    os.chmod(dest_file, Utils.O755)
                    copied_files.append(dest_file)

                else:
                    Logs.debug('android: No default splash screen override specified for "%s" orientation in %s' % (orientation_key, project))

                # process each of the resolution overrides
                warnings = []

                # The xxxhdpi resolution is only for application icons, its overkill to include them for drawables... for now
                valid_resolutions = set(RESOLUTION_SETTINGS)
                valid_resolutions.discard('xxxhdpi')

                for resolution in valid_resolutions:
                    target_directory = resource_node.make_node(orientation_path_prefix + '-' + resolution)

                    # get the current resolution splash image override
                    splash_img_source = oriented_splashes.get(resolution, default_splash_img)
                    if splash_img_source is default_splash_img:

                        # if both the resolution and the default are unspecified, warn the user but do nothing
                        if splash_img_source is None:
                            section = "%s-%s" % (orientation_key, resolution)
                            warnings.append(
                                'No splash screen override found for "{}".  Either supply one for "{}" or a "default" in the android_settings "splash_screen-{}" section of the project.json file for {}'.format(section, resolution, orientation_key, project)
                            )

                        # if only the resolution is unspecified, remove the resolution specific version from the project
                        else:
                            Logs.debug('android: Default splash screen being used for "%s-%s" in %s', orientation_key, resolution, project)
                            remove_file_and_empty_directory(target_directory.abspath(), APP_SPLASH_NAME)

                        continue

                    splash_img_source_node = construct_source_path(conf, project, splash_img_source)
                    splash_img_target_node = target_directory.make_node(APP_SPLASH_NAME)

                    shutil.copyfile(splash_img_source_node, splash_img_target_node.abspath())
                    splash_img_target_node.chmod(Utils.O755)
                    copied_files.append(splash_img_target_node.abspath())

                # guard against spamming warnings in the case the splash override block is full of garbage and no actual overrides
                if len(warnings) != len(valid_resolutions):
                    if unused_override_warning:
                        Logs.warn('[WARN] {}'.format(unused_override_warning))
                    else:
                        for warning_msg in warnings:
                            Logs.warn('[WARN] {}'.format(warning_msg))

        # micro-optimization to clear assets from the final bundle that won't be used
        if orientation == ORIENTATION_LANDSCAPE:
            Logs.debug('android: Clearing the portrait assets from %s' % project)
            clear_splash_assets(resource_node, 'drawable-port')

        elif orientation == ORIENTATION_PORTRAIT:
            Logs.debug('android: Clearing the landscape assets from %s' % project)
            clear_splash_assets(resource_node, 'drawable-land')

        # delete all files from the destination folder that were not copied by the script
        all_files = proj_root.ant_glob("**", excl=['wscript', 'build.gradle', '*.iml', 'assets_for_apk/*'])
        files_to_delete = [path for path in all_files if path.abspath() not in copied_files]

        for file in files_to_delete:
            file.delete()

    # add all the projects to the root wscript
    android_wscript = android_root.make_node('wscript')
    with open(android_wscript.abspath(), 'w') as wscript_file:
        w = wscript_file.write

        write_auto_gen_header(wscript_file)

        w('SUBFOLDERS = [\n')
        w('\t\'%s\'\n]\n\n' % '\',\n\t\''.join(created_directories))

        w('def build(bld):\n')
        w('\tvalid_subdirs = [x for x in SUBFOLDERS if bld.path.find_node("%s/wscript" % x)]\n')
        w('\tbld.recurse(valid_subdirs)\n')

    # Some Android SDK libraries have bugs, so we need to copy them locally and patch them.
    if not copy_and_patch_android_libraries(conf, source_node, android_root):
        return False

    return True


@conf
def process_android_projects(conf):
    conf.recurse(conf.get_android_project_relative_path())


################################################################
@conf
def is_module_for_game_project(self, module_name, game_project, project_name):
    """
    Check to see if the task generator is part of the build for a particular game project.
    The following rules apply:
        1. It is a gem requested by the game project
        2. It is the game project / project's launcher
        3. It is part of the general modules list
    """
    enabled_game_projects = self.get_enabled_game_project_list()

    if self.is_gem(module_name):
        gem_name_list = [gem.name for gem in self.get_game_gems(game_project)]
        return (True if module_name in gem_name_list else False)

    elif module_name == game_project or game_project == project_name:
        return True

    elif module_name not in enabled_game_projects and project_name is None:
        return True

    return False


################################################################
def collect_game_project_modules(ctx, game_project):

    module_tasks = []
    for group in ctx.groups:
        for task_generator in group:
            if not isinstance(task_generator, TaskGen.task_gen):
                continue

            Logs.debug('android: Processing task %s', task_generator.name)

            task_type = getattr(task_generator, '_type', None)
            if task_type not in ('stlib', 'shlib', 'objects'):
                Logs.debug('android:  -> Task is NOT a C/C++ task, Skipping...')
                continue

            project_name = getattr(task_generator, 'project_name', None)
            if not ctx.is_module_for_game_project(task_generator.name, game_project, project_name):
                Logs.debug('android:  -> Task is NOT part of the game project, Skipping...')
                continue

            module_tasks.append(task_generator)

    return module_tasks


def collect_source_paths(android_task, module_tasks, src_path_tag):

    game_project = android_task.game_project
    bld = android_task.bld

    platform = bld.env['PLATFORM']
    config = bld.env['CONFIGURATION']

    search_tags = [
        'android_{}'.format(src_path_tag),
        'android_{}_{}'.format(config, src_path_tag),

        '{}_{}'.format(platform, src_path_tag),
        '{}_{}_{}'.format(platform, config, src_path_tag),
    ]

    source_paths = []
    for task_generator in module_tasks:
        raw_paths = []
        for tag in search_tags:
            raw_paths += getattr(task_generator, tag, [])

        for path in raw_paths:
            if os.path.isabs(path):
                path = bld.root.make_node(path)
            else:
                path = task_generator.path.make_node(path)
            source_paths.append(path)

    return source_paths


################################################################
def set_key_and_store_pass(ctx):
    if ctx.get_android_build_environment() == 'Distribution':
        key_pass = ctx.options.distro_key_pass
        store_pass = ctx.options.distro_store_pass

        if not (key_pass and store_pass):
            ctx.fatal('[ERROR] Build environment is set to Distribution but --distro-key-pass or --distro-store-pass arguments were not specified or blank')

    else:
        key_pass = ctx.options.dev_key_pass
        store_pass = ctx.options.dev_store_pass

    ctx.env['KEYPASS'] = key_pass
    ctx.env['STOREPASS'] = store_pass


################################################################
################################################################
class strip_debug_symbols_base(Task):
    """
    Strips the debug symbols from a shared library
    """
    color = 'CYAN'
    vars = [ 'STRIP' ]

    def __str__(self):
        task_description = super(strip_debug_symbols_base, self).__str__()
        return task_description.replace(self.__class__.__name__, 'strip_debug_symbols')

    def runnable_status(self):
        if super(strip_debug_symbols_base, self).runnable_status() == ASK_LATER:
            return ASK_LATER

        src = self.inputs[0].abspath()
        tgt = self.outputs[0].abspath()

        # If the target file is missing, we need to run
        try:
            stat_tgt = os.stat(tgt)
        except OSError:
            return RUN_ME

        # Now compare both file stats
        try:
            stat_src = os.stat(src)
        except OSError:
            pass
        else:
            CREATION_TIME_PADDING = 10

            # only check timestamps
            if stat_src.st_mtime >= (stat_tgt.st_mtime + CREATION_TIME_PADDING):
                return RUN_ME

        # Everything fine, we can skip this task
        return SKIP_ME


class strip_debug_symbols_gcc(strip_debug_symbols_base):
    run_str = '${STRIP} --strip-debug -o ${TGT} ${SRC}'

class strip_debug_symbols_llvm_ndk_r18(strip_debug_symbols_base):
    run_str = '${STRIP} -strip-debug ${SRC} ${TGT}'

class strip_debug_symbols_llvm_ndk_r19(strip_debug_symbols_base):
    run_str = '${STRIP} ${SRC} -strip-debug -o ${TGT}'


@taskgen_method
def create_strip_debug_symbols_task(self, src, tgt):
    # ndk r18 introduced the llvm based symbol stripper and in r19 they changed the usage
    ndk_rev = self.env['ANDROID_NDK_REV_MAJOR']

    if ndk_rev >= 19:
        symbol_stripper = 'strip_debug_symbols_llvm_ndk_r19'
    if ndk_rev == 18:
        symbol_stripper = 'strip_debug_symbols_llvm_ndk_r18'
    else:
        symbol_stripper = 'strip_debug_symbols_gcc'

    return self.create_task(symbol_stripper, src, tgt)


################################################################
class android_manifest_preproc(Task):
    color = 'PINK'

    def run(self):
        min_sdk = self.env['ANDROID_NDK_PLATFORM_NUMBER']
        target_sdk = self.env['ANDROID_SDK_VERSION_NUMBER']

        input_contents = self.inputs[0].read()
        transfromed_text = input_contents.replace(
            '<!-- SDK_VERSIONS -->',
            '<uses-sdk android:minSdkVersion="{}" android:targetSdkVersion="{}"/>'.format(min_sdk, target_sdk))
        self.outputs[0].write(transfromed_text)


################################################################
class aapt_package_base(Task):
    """
    General purpose 'package' variant Android Asset Packaging Tool task
    """
    color = 'PINK'
    vars = [ 'AAPT', 'AAPT_RESOURCES', 'AAPT_INCLUDES', 'AAPT_PACKAGE_FLAGS' ]


    def runnable_status(self):

        def _to_list(value):
            if isinstance(value, list):
                return value
            else:
                return [ value ]

        if not self.inputs:
            self.inputs  = []

            aapt_resources = getattr(self.generator, 'aapt_resources', [])
            assets = getattr(self, 'assets', [])
            apk_layout = getattr(self, 'srcdir', [])

            input_paths = _to_list(aapt_resources) + _to_list(assets) + _to_list(apk_layout)

            for path in input_paths:
                files = path.ant_glob('**/*')
                self.inputs.extend(files)

            android_manifest = getattr(self.generator, 'main_android_manifest', None)
            if android_manifest:
                self.inputs.append(android_manifest)

        result = super(aapt_package_base, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
class android_code_gen(aapt_package_base):
    """
    Generates the R.java files from the Android resources
    """
    run_str = '${AAPT} package -f -M ${ANDROID_MANIFEST} ${AAPT_RESOURCE_ST:AAPT_RESOURCES} ${AAPT_INLC_ST:AAPT_INCLUDES} ${AAPT_PACKAGE_FLAGS} -m -J ${OUTDIR}'


################################################################
class package_resources(aapt_package_base):
    """
    Packages all the native resources from the Android project
    """
    run_str = '${AAPT} package -f ${ANDROID_DEBUG_MODE} -M ${ANDROID_MANIFEST} ${AAPT_RESOURCE_ST:AAPT_RESOURCES} ${AAPT_INLC_ST:AAPT_INCLUDES} ${AAPT_PACKAGE_FLAGS} -F ${TGT}'


################################################################
class build_apk(aapt_package_base):
    """
    Generates an unsigned, unaligned Android APK
    """
    run_str = '${AAPT} package -f ${ANDROID_DEBUG_MODE} -M ${ANDROID_MANIFEST} ${AAPT_RESOURCE_ST:AAPT_RESOURCES} ${AAPT_INLC_ST:AAPT_INCLUDES} ${AAPT_ASSETS_ST:AAPT_ASSETS} ${AAPT_PACKAGE_FLAGS} -F ${TGT} ${SRCDIR}'


################################################################
class aapt_crunch(Task):
    """
    Processes the PNG resources from the Android project
    """
    color = 'PINK'
    run_str = '${AAPT} crunch ${AAPT_RESOURCE_ST:AAPT_RESOURCES} -C ${TGT}'
    vars = [ 'AAPT', 'AAPT_RESOURCES' ]


    def runnable_status(self):
        if not self.inputs:
            self.inputs  = []

            for resource in self.generator.aapt_resources:
                res = resource.ant_glob('**/*')
                self.inputs.extend(res)

        return super(aapt_crunch, self).runnable_status()


################################################################
class aidl(Task):
    """
    Processes the Android interface files
    """
    color = 'PINK'
    run_str = '${AIDL} ${AIDL_PREPROC_ST:AIDL_PREPROCESSES} ${SRC} ${TGT}'


    def runnable_status(self):
        result = super(aidl, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
class dex(Task):
    """
    Compiles the .class files into the dalvik executable
    """
    color = 'PINK'
    run_str = '${DX} --dex --output ${TGT} ${JAR_INCLUDES} ${SRCDIR}'


    def runnable_status(self):

        for tsk in self.run_after:
            if not tsk.hasrun:
                return ASK_LATER

        if not self.inputs:
            self.inputs  = []

            srcdir = self.srcdir
            if not isinstance(srcdir, list):
                srcdir = [ srcdir ]

            for src_node in srcdir:
                self.inputs.extend(src_node.ant_glob('**/*.class', remove = False, quiet = True))

        result = super(dex, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
class zipalign(Task):
    """
    Performs a specified byte alignment on the source file
    """
    color = 'PINK'
    run_str = '${ZIPALIGN} -f ${ZIPALIGN_SIZE} ${SRC} ${TGT}'

    def runnable_status(self):
        result = super(zipalign, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


################################################################
################################################################
@taskgen_method
def create_debug_strip_task(self, source_file, dest_location):

    lib_name = os.path.basename(source_file.abspath())
    output_node = dest_location.make_node(lib_name)

    # For Android Studio we should just copy the libs because stripping is part of the build process.
    # But we have issues with long path names that makes the stripping process to fail in Android Studio.
    self.create_strip_debug_symbols_task(source_file, output_node)


################################################################
@feature('cshlib', 'cxxshlib')
@before_method('propagate_uselib_vars')
def link_aws_sdk_core_after_android(self):
    '''
    Android monolithic builds require the aws core library to be linked after any
    other aws libs in order for the symbols to be resolved correctly.
    '''
    if not (self.bld.is_android_platform(self.env['PLATFORM']) and self.bld.spec_monolithic_build()):
        return

    if 'AWS_CPP_SDK_CORE' in self.uselib:
        self.uselib = [ uselib for uselib in self.uselib if uselib != 'AWS_CPP_SDK_CORE' ]
        self.uselib.append('AWS_CPP_SDK_CORE')


@feature('c', 'cxx')
@after_method('apply_link')
def android_natives_processing(self):

    bld = self.bld
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    link_task = getattr(self, 'link_task', None)

    if (not bld.is_android_platform(platform)) or (not link_task) or (self._type != 'shlib'):
        return

    output_node = self.bld.get_output_folders(platform, configuration)[0]
    project_name = getattr(self, 'project_name', None)

    game_project_builder_nodes = []
    for game in bld.get_enabled_game_project_list():
        if not bld.is_module_for_game_project(self.name, game, project_name):
            continue

        # If the game is a valid android project, a specific build native value will have been created during
        # the project configuration.  Only process games with valid android project settings
        game_build_native_key = '{}_BUILDER_NATIVES'.format(game)
        if game_build_native_key not in self.env:
            continue

        builder_node = bld.root.find_dir(self.env[game_build_native_key])
        if builder_node:
            game_project_builder_nodes.append(builder_node)


    for src in link_task.outputs:
        src_lib = output_node.make_node(src.name)
        for builder_node in game_project_builder_nodes:
            self.create_debug_strip_task(src_lib, builder_node)


    third_party_artifacts = getattr(self.env, 'COPY_3RD_PARTY_ARTIFACTS', [])
    for artifact in third_party_artifacts:
        _, ext = os.path.splitext(artifact.abspath())
        # Only care about shared libraries
        if ext != '.so':
            continue

        for builder_node in game_project_builder_nodes:
            self.create_debug_strip_task(artifact, builder_node)


################################################################
################################################################
@feature('wrapped_copy_outputs')
@before_method('process_source')
def create_copy_outputs(self):
    self.meths.remove('process_source')
    self.create_task('copy_outputs', self.source, self.target)


@taskgen_method
def sign_and_align_apk(self, base_apk_name, raw_apk, intermediate_folder, final_output, suffix = ''):
    # first sign the apk with jarsigner
    apk_name = '{}_unaligned{}.apk'.format(base_apk_name, suffix)
    unaligned_apk = intermediate_folder.make_node(apk_name)

    self.jarsign_task = jarsign_task = self.create_task('jarsigner', raw_apk, unaligned_apk)

    # align the new apk with assets
    apk_name = '{}{}.apk'.format(base_apk_name, suffix)
    final_apk = final_output.make_node(apk_name)

    self.zipalign_task = zipalign_task = self.create_task('zipalign', unaligned_apk, final_apk)

    # chain the alignment to happen after signing
    zipalign_task.set_run_after(jarsign_task)


################################################################
################################################################
@conf
def AndroidAPK(ctx, *k, **kw):

    project_name = kw['project_name']

    env = ctx.env

    platform = env['PLATFORM']
    configuration = env['CONFIGURATION']

    if ctx.cmd in ('configure', 'generate_uber_files', 'generate_module_def_files', 'msvs'):
        return
    if not (ctx.is_android_platform(platform) or platform =='project_generator'):
        return
    if project_name not in ctx.get_enabled_game_project_list():
        return

    Logs.debug('android: ******************************** ')
    Logs.debug('android: Processing {}...'.format(project_name))

    if not hasattr(ctx, 'default_group_name'):
        default_group = ctx.get_group(None)
        ctx.default_group_name = ctx.get_group_name(default_group)
    Logs.debug('android: ctx.default_group_name = %s', ctx.default_group_name)

    root_input = ctx.path.get_src().make_node('src')
    root_output = ctx.path.get_bld()

    apk_layout_dir = root_output.make_node('builder')

    # The variant name is constructed in the same fashion as how Gradle generates all it's build
    # variants.  After all the Gradle configurations and product flavors are evaluated, the variants
    # are generated in the following lower camel case format {product_flavor}{configuration}.
    # Our configuration and Gradle's configuration is a one to one mapping of what each describe,
    # while our platform is effectively Gradle's product flavor.
    gradle_variant = '{}{}'.format(platform, configuration.title())

    # copy over the required 3rd party libs that need to be bundled into the apk
    abi = env['ANDROID_ARCH']
    if not abi:
        abi = 'armeabi-v7a'

    if ctx.options.from_android_studio:
        local_native_libs_node = root_input.make_node([ gradle_variant, 'jniLibs', abi ])
    else:
        local_native_libs_node = apk_layout_dir.make_node([ 'lib', abi ])

    local_native_libs_node.mkdir()
    Logs.debug('android: APK builder path (native libs) -> {}'.format(local_native_libs_node.abspath()))
    env['{}_BUILDER_NATIVES'.format(project_name)] = local_native_libs_node.abspath()

    android_cache = get_android_cache_node(ctx)
    libs_to_copy = env['EXT_LIBS']
    for lib in libs_to_copy:

        src = android_cache.make_node(lib)

        lib_name = os.path.basename(lib)
        tgt = local_native_libs_node.make_node(lib_name)

        ctx(features = 'wrapped_copy_outputs', source = src, target = tgt)

    # since we are having android studio building the apk we can kick out early
    if ctx.options.from_android_studio:
        return

    # order of precedence from highest (primary) to lowest (inputs): full variant, build type, product flavor, main
    local_variant_dirs = [ gradle_variant, configuration, platform, 'main' ]

    java_source_nodes = []

    android_manifests = []
    resource_nodes = []

    for source_dir in local_variant_dirs:

        java_node = root_input.find_node([ source_dir, 'java' ])
        if java_node:
            java_source_nodes.append(java_node)

        res_node = root_input.find_node([ source_dir, 'res' ])
        if res_node:
            resource_nodes.append(res_node)

        manifest_node = root_input.find_node([ source_dir, 'AndroidManifest.xml' ])
        if manifest_node:
            android_manifests.append(manifest_node)

    if not android_manifests:
        ctx.fatal('[ERROR] Unable to find any AndroidManifest.xml files in project path {}.'.format(ctx.path.get_src().abspath()))

    Logs.debug('android: Found local Java source directories {}'.format(java_source_nodes))
    Logs.debug('android: Found local resource directories {}'.format(resource_nodes))
    Logs.debug('android: Found local manifest file {}'.format(android_manifests))

    # get the keystore passwords
    set_key_and_store_pass(ctx)

    # Private function to add android libraries to the build
    def _add_library(folder, libName, source_paths, manifests, package_names, resources):
        '''
        Collect the resources and package names of the specified library.
        '''
        if not folder:
            Logs.error('[ERROR] Invalid folder for library {}. Please check the path in the {} file.'.format(libName, java_libs_json.abspath()))
            return False

        src = folder.find_dir('src')
        if not src:
            Logs.error("[ERROR] Could not find the 'src' folder for library {}. Please check that they are present at {}".format(libName, folder.abspath()))
            return False

        source_paths.append(src)

        manifest =  folder.find_node('AndroidManifest.xml')
        if not manifest:
            Logs.error("[ERROR] Could not find the AndroidManifest.xml folder for library {}. Please check that they are present at {}".format(libName, folder.abspath()))
            return False

        manifests.append(manifest)

        tree = ET.parse(manifest.abspath())
        root = tree.getroot()
        package = root.get('package')
        if not package:
            Logs.error("[ERROR] Could not find 'package' node in {}. Please check that the manifest is valid ".format(manifest.abspath()))
            return False

        package_names.append(package)

        res = folder.find_dir('res')
        if res:
            resources.append(res)

        return True

    library_packages = []
    library_jars = []

    java_libs_json = ctx.root.make_node(kw['android_java_libs'])
    json_data = ctx.parse_json_file(java_libs_json)
    if json_data:
        for libName, value in json_data.iteritems():
            if 'libs' in value:
                # Collect any java lib that is needed so we can add it to the classpath.
                for java_lib in value['libs']:
                    jar_path = string.Template(java_lib['path']).substitute(env)
                    if os.path.exists(jar_path):
                        library_jars.append(jar_path)
                    elif java_lib['required']:
                        ctx.fatal('[ERROR] Required java lib - {} - was not found'.format(jar_path))

            if 'patches' in value:
                cur_path = ctx.srcnode.abspath()
                rel_path = ctx.get_android_patched_libraries_relative_path()
                lib_path = os.path.join(cur_path, rel_path, libName)
            else:
                # Search the multiple library paths where the library can be
                lib_path = None
                for path in value['srcDir']:
                    path = string.Template(path).substitute(env)
                    if os.path.exists(path):
                        lib_path = path
                        break

            if not _add_library(ctx.root.make_node(lib_path), libName, java_source_nodes, android_manifests, library_packages, resource_nodes):
                ctx.fatal('[ERROR] Could not add the android library - {}'.format(libName))

    r_java_out = root_output.make_node('r')
    aidl_out = root_output.make_node('aidl')

    java_out = root_output.make_node('classes')
    crunch_out = root_output.make_node('res')

    manifests_out = root_output.make_node('manifest')

    game_package = ctx.get_android_package_name(project_name)
    executable_name = ctx.get_executable_name(project_name)

    Logs.debug('android: ****')
    Logs.debug('android: All Java source directories {}'.format(java_source_nodes))
    Logs.debug('android: All resource directories {}'.format(resource_nodes))

    java_include_paths = java_source_nodes + [ r_java_out, aidl_out ]
    java_source_paths = java_source_nodes

    uses = kw.get('use', [])
    if not isinstance(uses, list):
        uses = [ uses ]

    ################################
    # Push all the Android apk packaging into their own build groups with
    # lazy posting to ensure they are processed at the end of the build
    ctx.post_mode = POST_LAZY
    build_group_name = '{}_android_group'.format(project_name)
    ctx.add_group(build_group_name)

    ctx(
        name = '{}_APK'.format(project_name),
        target = executable_name,

        features = [ 'android', 'android_apk', 'javac', 'use', 'uselib' ],
        use = uses,

        game_project = project_name,

        # java settings
        compat          = env['JAVA_VERSION'],  # java compatibility version number
        classpath       = library_jars,

        srcdir          = java_include_paths,   # folder containing the sources to compile
        outdir          = java_out,             # folder where to output the classes (in the build directory)

        # android settings
        android_manifests = android_manifests,
        android_package = game_package,

        aidl_outdir = aidl_out,
        r_java_outdir = r_java_out,

        manifests_out = manifests_out,

        apk_layout_dir = apk_layout_dir,
        apk_native_lib_dir = local_native_libs_node,
        apk_output_dir = 'apk',

        aapt_assets = [],
        aapt_resources = resource_nodes,

        aapt_extra_packages = library_packages,

        aapt_package_flags = [],
        aapt_package_resources_outdir = 'bin',

        aapt_crunch_outdir = crunch_out,
    )

    # reset the build group/mode back to default
    ctx.post_mode = POST_AT_ONCE
    ctx.set_group(ctx.default_group_name)


################################################################
@feature('android')
@before('apply_java')
def apply_android_java(self):
    """
    Generates the AIDL tasks for all other tasks that may require it, and adds
    their Java source directories to the current projects Java source paths
    so they all get processed at the same time.  Also processes the direct
    Android Archive Resource uses.
    """

    Utils.def_attrs(
        self,

        srcdir = [],
        classpath = [],

        aidl_srcdir = [],

        aapt_assets = [],
        aapt_includes = [],
        aapt_resources = [],
        aapt_extra_packages = [],
    )

    # validate we have some required attributes
    apk_native_lib_dir = getattr(self, 'apk_native_lib_dir', None)
    if not apk_native_lib_dir:
        self.fatal('[ERROR] No "apk_native_lib_dir" specified in Android package task.')

    android_manifests = getattr(self, 'android_manifests', None)
    if not android_manifests:
        self.fatal('[ERROR] No "android_manifests" specified in Android package task.')

    manifest_nodes = []

    for manifest in android_manifests:
        if not isinstance(manifest, Node.Node):
            manifest_nodes.append(self.path.get_src().make_node(manifest))
        else:
            manifest_nodes.append(manifest)

    self.android_manifests = manifest_nodes
    self.main_android_manifest = manifest_nodes[0] # guaranteed to be the main; manifests are added in order of precedence highest to lowest

    # process the uses
    if not hasattr(self, 'use'):
        setattr(self, 'use', [])

    local_uses = self.to_list(self.use)[:]
    Logs.debug('android: -> Processing Android libs used by %s: %s', self.name, local_uses)

    # collect any AAR uses from other modules
    game_project_modules = collect_game_project_modules(self.bld, self.game_project)

    other_uses = []
    for tsk in game_project_modules:
        # skip the launchers / same module, those source paths were already added above
        if tsk.name.endswith('AndroidLauncher'):
            continue

        other_uses.extend(self.to_list(getattr(tsk, 'use', [])))

    other_uses = list(set(other_uses))
    Logs.debug('android: -> Processing possible Android libs used by dependent modules: %s', other_uses)

    input_manifests = []
    use_libs_added = []

    libs = local_uses + other_uses
    for lib_name in libs:
        try:
            task_gen = self.bld.get_tgen_by_name(lib_name)
            task_gen.post()
        except Exception:
            continue
        else:
            if not hasattr(task_gen, 'aar_task'):
                continue

            Logs.debug('android: -> Applying AAR - %s - to the build', lib_name)
            use_libs_added.append(lib_name)

            # ensure the lib is part of the APK uses so all the AAR properties are propagated
            if lib_name not in local_uses:
                append_to_unique_list(self.use, lib_name)

            # required entries from the library
            append_to_unique_list(self.aapt_extra_packages, task_gen.package)
            append_to_unique_list(self.aapt_includes, task_gen.jar_task.outputs[0].abspath())
            append_to_unique_list(self.aapt_resources, task_gen.aapt_resources)

            append_to_unique_list(input_manifests, task_gen.manifest)

            # optional entries from the library
            if task_gen.aapt_assets:
                append_to_unique_list(self.aapt_assets, task_gen.aapt_assets)

            # since classpath is propagated by the java tool, we just need to make sure the jars are propagated to the android specific tools using aapt_includes
            if task_gen.classpath:
                append_to_unique_list(self.aapt_includes, task_gen.classpath)

            if task_gen.native_libs:
                native_libs_root = task_gen.native_libs_root
                native_libs = task_gen.native_libs

                for lib in native_libs:
                    rel_path = lib.path_from(native_libs_root)

                    tgt = apk_native_lib_dir.make_node(rel_path)

                    strip_task = self.create_strip_debug_symbols_task(lib, tgt)
                    self.bld.add_to_group(strip_task, self.bld.default_group_name)

    Logs.debug('android: -> Android use libs added {}'.format(use_libs_added))

    # generate the task to merge the manifests
    manifest_nodes.extend(input_manifests)

    # manifest processing
    manifests_out = getattr(self, 'manifests_out', None)
    if manifests_out:
        if not isinstance(manifests_out, Node.Node):
            manifests_out = self.path.get_bld().make_node(manifests_out)
    else:
        manifests_out = self.path.get_bld().make_node('manifest')
    manifests_out.mkdir()

    # android studio doesn't like having the min/target sdk specified in the manifest
    # but then the manifest merger will complain with it gone, so it needs to be injected
    # back in before the merging happens
    manifest_preproc_out = manifests_out.make_node('preproc')
    manifest_preproc_out.mkdir()

    manifest_preproc_tgt = manifest_preproc_out.make_node('AndroidManifest.xml')
    self.manifest_preproc_task = self.create_task('android_manifest_preproc', self.main_android_manifest, manifest_preproc_tgt)
    self.main_android_manifest = manifest_nodes[0] = manifest_preproc_tgt

    if len(manifest_nodes) >= 2:
        manifest_merged_out = manifests_out.make_node('merged')
        manifest_merged_out.mkdir()

        merged_manifest = manifest_merged_out.make_node('AndroidManifest.xml')
        self.manifest_merger_task = manifest_merger_task = self.create_task('android_manifest_merger', manifest_nodes, merged_manifest)

        manifest_merger_task.env['MAIN_MANIFEST'] = self.main_android_manifest.abspath()

        input_manifest_paths = [ manifest.abspath() for manifest in manifest_nodes[1:] ]
        manifest_merger_task.env['LIBRARY_MANIFESTS'] = os.pathsep.join(input_manifest_paths)

        self.main_android_manifest = merged_manifest

    # generate all the aidl tasks
    aidl_outdir = getattr(self, 'aidl_outdir', None)
    if aidl_outdir:
        if not isinstance(aidl_outdir, Node.Node):
            aidl_outdir = self.path.get_bld().make_node(aidl_outdir)
    else:
        aidl_outdir = self.path.get_bld().make_node('aidl')
    aidl_outdir.mkdir()

    aidl_src_paths = collect_source_paths(self, game_project_modules, 'aidl_src_path')
    self.aidl_tasks = []

    for srcdir in aidl_src_paths:
        for aidl_file in srcdir.ant_glob('**/*.aidl'):
            rel_path = aidl_file.path_from(srcdir)

            java_file = aidl_outdir.make_node('{}.java'.format(os.path.splitext(rel_path)[0]))

            aidl_task = self.create_task('aidl', aidl_file, java_file)
            self.aidl_tasks.append(aidl_task)

    java_src_paths = collect_source_paths(self, game_project_modules, 'java_src_path')
    append_to_unique_list(self.srcdir, java_src_paths)

    jars = collect_source_paths(self, game_project_modules, 'jars')
    append_to_unique_list(self.classpath, jars)

    Logs.debug('android: -> Additional Java source paths found {}'.format(java_src_paths))


################################################################
@feature('android')
@before_method('process_source')
@after_method('apply_java')
def apply_android(self):
    """
    Generates the code generation task (produces R.java) and setups the task chaining
    for AIDL, Java and the code gen task
    """

    Utils.def_attrs(
        self,

        classpath = [],

        aapt_resources = [],
        aapt_includes = [],
        aapt_extra_packages = [],
        aapt_package_flags = [],
    )

    main_package = getattr(self, 'android_package', None)
    if not main_package:
        self.fatal('[ERROR] No "android_package" specified in Android package task.')

    javac_task = getattr(self, 'javac_task', None)
    if not javac_task:
        self.fatal('[ERROR] It seems the "javac" task failed to be generated, unable to complete the Android build process.')

    self.code_gen_task = code_gen_task = self.create_task('android_code_gen')

    r_java_outdir = getattr(self, 'r_java_outdir', None)
    if r_java_outdir:
        if not isinstance(r_java_outdir, Node.Node):
            r_java_outdir = self.path.get_bld().make_node(r_java_outdir)
    else:
        r_java_outdir = self.path.get_bld().make_node('r')
    r_java_outdir.mkdir()

    code_gen_task.env['OUTDIR'] = r_java_outdir.abspath()

    android_manifest = self.main_android_manifest
    code_gen_task.env['ANDROID_MANIFEST'] = android_manifest.abspath()

    # resources

    aapt_resources = []
    for resource in self.aapt_resources:
        if isinstance(resource, Node.Node):
            aapt_resources.append(resource.abspath())
        else:
            aapt_resources.append(resource)

    self.aapt_resource_paths = aapt_resources
    code_gen_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    # included jars

    aapt_includes = self.aapt_includes + self.classpath

    aapt_include_paths = []

    for include_path in self.aapt_includes:
        if isinstance(include_path, Node.Node):
            aapt_include_paths.append(include_path.abspath())
        else:
            aapt_include_paths.append(include_path)

    self.aapt_include_paths = aapt_include_paths
    code_gen_task.env.append_value('AAPT_INCLUDES', aapt_include_paths)

    # additional flags

    aapt_package_flags = self.aapt_package_flags

    extra_packages = self.aapt_extra_packages
    if extra_packages:
        aapt_package_flags.extend([ '--extra-packages', ':'.join(extra_packages) ])

    code_gen_task.env.append_value('AAPT_PACKAGE_FLAGS', aapt_package_flags)

    # outputs (R.java files)

    included_packages = [ main_package ] + extra_packages

    output_nodes = []
    for package in included_packages:
        sub_dirs = package.split('.')
        dir_path = os.path.join(*sub_dirs)

        r_java_path = os.path.join(dir_path, 'R.java')
        r_java_node = r_java_outdir.make_node(r_java_path)

        output_nodes.append(r_java_node)

    code_gen_task.set_outputs(output_nodes)

    # task chaining

    manifest_preproc_task = getattr(self, 'manifest_preproc_task', None)
    manifest_merger_task = getattr(self, 'manifest_merger_task', None)

    if manifest_preproc_task and manifest_merger_task:
        code_gen_task.set_run_after(manifest_merger_task)
        manifest_merger_task.set_run_after(manifest_preproc_task)

    elif manifest_preproc_task:
        code_gen_task.set_run_after(manifest_preproc_task)

    elif manifest_merger_task:
        code_gen_task.set_run_after(manifest_merger_task)

    aidl_tasks = getattr(self, 'aidl_tasks', [])
    for aidl_task in aidl_tasks:
        code_gen_task.set_run_after(aidl_task)

    javac_task.set_run_after(self.code_gen_task)


################################################################
@feature('android_apk')
@after_method('apply_android')
def apply_android_apk(self):
    """
    Generates the rest of the tasks necessary for building an APK (dex, crunch, package, build, sign, alignment).
    """

    Utils.def_attrs(
        self,

        aapt_assets = [],
        aapt_include_paths = [],
        aapt_resource_paths = [],

        aapt_package_flags = [],
    )

    root_input = self.path.get_src()
    root_output = self.path.get_bld()

    if not hasattr(self, 'target'):
        self.target = self.name
    executable_name = self.target

    aapt_resources = self.aapt_resource_paths
    aapt_includes = self.aapt_include_paths

    aapt_assets = []
    asset_nodes = []
    for asset_dir in self.aapt_assets:
        if isinstance(asset_dir, Node.Node):
            aapt_assets.append(asset_dir.abspath())
            asset_nodes.append(asset_dir)
        else:
            aapt_assets.append(asset_dir)
            asset_nodes.append(root_input.make_node(asset_dir))

    android_manifest = self.android_manifests[0]
    if hasattr(self, 'manifest_merger_task'):
        android_manifest = self.manifest_merger_task.outputs[0]

    # dex task

    apk_layout_dir = getattr(self, 'apk_layout_dir', None)
    if apk_layout_dir:
        if not isinstance(apk_layout_dir, Node.Node):
            apk_layout_dir = self.path.get_bld().make_node(apk_layout_dir)
    else:
        apk_layout_dir = root_output.make_node('builder')

    apk_layout_dir.mkdir()

    self.dex_task = dex_task = self.create_task('dex')
    self.dex_task.set_outputs(apk_layout_dir.make_node('classes.dex'))

    dex_task.env.append_value('JAR_INCLUDES', aapt_includes)

    dex_srcdir = self.outdir
    dex_task.env['SRCDIR'] = dex_srcdir.abspath()
    dex_task.srcdir = dex_srcdir

    # crunch task

    self.crunch_task = crunch_task = self.create_task('aapt_crunch')

    crunch_outdir = getattr(self, 'aapt_crunch_outdir', None)
    if crunch_outdir:
        if not isinstance(crunch_outdir, Node.Node):
            crunch_outdir = root_output.make_node(crunch_outdir)
    else:
        crunch_outdir = root_output.make_node('res')
    crunch_outdir.mkdir()

    crunch_task.set_outputs(crunch_outdir)

    crunch_task.env.append_value('AAPT_INCLUDES', aapt_includes)
    crunch_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    # package resources task

    self.package_resources_task = package_resources_task = self.create_task('package_resources')

    aapt_package_resources_outdir = getattr(self, 'aapt_package_resources_outdir', None)
    if aapt_package_resources_outdir:
        if not isinstance(aapt_package_resources_outdir, Node.Node):
            aapt_package_resources_outdir = root_output.make_node(aapt_package_resources_outdir)
    else:
        aapt_package_resources_outdir = root_output.make_node('bin')
    aapt_package_resources_outdir.mkdir()

    package_resources_task.set_outputs(aapt_package_resources_outdir.make_node('{}.ap_'.format(executable_name)))

    package_resources_task.env['ANDROID_MANIFEST'] = android_manifest.abspath()

    package_resources_task.env.append_value('AAPT_INCLUDES', aapt_includes)
    package_resources_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    ################################
    # generate the APK

    # Generating all the APK has to be in the right order.  This is important for Android store APK uploads,
    # if the alignment happens before the signing, then the signing will blow over the alignment and will
    # require a realignment before store upload.
    # 1. Generate the unsigned, unaligned APK
    # 2. Sign the APK
    # 3. Align the APK

    apk_output_dir = getattr(self, 'apk_output_dir', None)
    if apk_output_dir:
        if not isinstance(apk_output_dir, Node.Node):
            apk_output_dir = root_output.make_node(apk_output_dir)
    else:
        apk_output_dir = root_output.make_node('apk')
    apk_output_dir.mkdir()

    # 1. build_apk
    self.apk_task = apk_task = self.create_task('build_apk')

    apk_task.env['SRCDIR'] = apk_layout_dir.abspath()
    apk_task.srcdir = apk_layout_dir

    apk_name = '{}_unaligned_unsigned.apk'.format(executable_name)
    unsigned_unaligned_apk = apk_output_dir.make_node(apk_name)
    apk_task.set_outputs(unsigned_unaligned_apk)

    apk_task.env['ANDROID_MANIFEST'] = android_manifest.abspath()

    apk_task.assets = asset_nodes
    apk_task.env.append_value('AAPT_ASSETS', aapt_assets)

    apk_task.env.append_value('AAPT_INCLUDES', aapt_includes)
    apk_task.env.append_value('AAPT_RESOURCES', aapt_resources)

    # 2. jarsign and 3. zipalign
    final_apk_out = self.bld.get_output_folders(self.bld.env['PLATFORM'], self.bld.env['CONFIGURATION'])[0]
    self.sign_and_align_apk(
        executable_name, # base_apk_name
        unsigned_unaligned_apk, # raw_apk
        apk_output_dir, # intermediate_folder
        final_apk_out # final_output
    )

    # task chaining
    dex_task.set_run_after(self.javac_task)

    crunch_task.set_run_after(dex_task)
    package_resources_task.set_run_after(crunch_task)

    apk_task.set_run_after(package_resources_task)
    self.jarsign_task.set_run_after(apk_task)


###############################################################################
###############################################################################
def adb_call(*cmd_args, **keywords):
    '''
    Issue a adb command. Args are joined into a single string with spaces
    in between and keyword arguments supported is device=serial # of device
    reported by adb.
    Examples:
    adb_call('start-server') results in "adb start-server" being executed
    adb_call('push', local_path, remote_path, device='123456') results in "adb -s 123456 push <local_path> <remote_path>" being executed
    '''

    global ADB_QUOTED_PATH
    command = [ ADB_QUOTED_PATH ]

    if 'device' in keywords:
        command.extend([ '-s', keywords['device'] ])

    command.extend(cmd_args)

    cmdline = ' '.join(command)
    Logs.debug('adb_call: running command \'%s\'', cmdline)

    try:
        output = check_output(cmdline, stderr = STDOUT, shell = True)
        stripped_output = output.rstrip()

        # don't need to dump the output of 'push' or 'install' commands
        if not any(cmd for cmd in ('push', 'install') if cmd in cmd_args):
            if '\n' in stripped_output:
                # the newline arg is because Logs.debug will replace newlines with spaces
                # in the format string before passing it on to the logger
                Logs.debug('adb_call: output = %s%s', '\n', stripped_output)
            else:
                Logs.debug('adb_call: output = %s', stripped_output)

        return stripped_output

    except Exception as inst:
        Logs.debug('adb_call: exception was thrown: ' + str(inst))
        return None  # Return None so the caller can handle the failure gracefully'


def adb_ls(path, device_id, args = [], as_root = False):
    '''
    Special wrapper around calling "adb shell ls <args> <path>".  This uses
    adb_call under the hood but provides some additional error handling specific
    to the "ls" command.  Optionally, this command can be run as super user, or
    'as_root', which is disabled by default.
    Returns:
        status - if the command failed or not
        output - the stripped output from the ls command
    '''
    error_messages = [
        'No such file or directory',
        'Permission denied'
    ]

    shell_command = [ 'shell' ]

    if as_root:
        shell_command.extend([ 'su', '-c' ])

    shell_command.append('ls')
    shell_command.extend(args)
    shell_command.append(path)

    Logs.debug('adb_ls: {}'.format(shell_command))
    raw_output = adb_call(*shell_command, device = device_id)

    if raw_output is None:
        Logs.debug('adb_ls: No output given')
        return False, None

    if raw_output is None or any([error for error in error_messages if error in raw_output]):
        Logs.debug('adb_ls: Error message found')
        status = False
    else:
        Logs.debug('adb_ls: Command was successful')
        status = True

    return status, raw_output


def get_list_of_android_devices():
    '''
    Gets the connected android devices using the adb command devices and
    returns a list of serial numbers of connected devices.
    '''
    devices = []
    devices_output = adb_call("devices")
    if devices_output is not None:

        devices_output = devices_output.split(os.linesep)
        for output in devices_output:

            if any(x in output for x in ['List', '*', 'emulator']):
                Logs.debug("android: skipping the following line as it has 'List', '*' or 'emulator' in it: %s" % output)
                continue

            device_serial = output.split()
            if device_serial:
                if 'unauthorized' in output.lower():
                    Logs.warn("[WARN] android: device %s is not authorized for development access. Please reconnect the device and check for a confirmation dialog." % device_serial[0])

                else:
                    devices.append(device_serial[0])
                    Logs.debug("android: found device with serial: " + device_serial[0])

    return devices


def get_device_access_type(device_id):
    '''
    Determines what kind of access level we have on the device
    '''
    adb_call('root', device = device_id) # this ends up being either a no-op or restarts adbd on device as root

    adbd_info = adb_call('shell', '"ps | grep adbd"', device = device_id)
    if adbd_info and ('root' in adbd_info):
        Logs.debug('adb_call: Device - {} - has adbd root access'.format(device_id))
        return ACCESS_ROOT_ADBD

    su_test = adb_call('shell', '"su -c echo test"', device = device_id)
    if su_test and ('test' in su_test):
        Logs.debug('adb_call: Device - {} - has shell su access'.format(device_id))
        return ACCESS_SHELL_SU

    Logs.debug('adb_call: Unable to verify root access for device {}.  Assuming default access mode.'.format(device_id))
    return ACCESS_NORMAL


def update_device_file_timestamp(remote_file_path, device_id, as_root = False):
    '''
    Updates the contents of the remote file with the current local time. Optionally, this
    command can be run as super user, or 'as_root', which is disabled by default.
    '''
    adb_command = [ 'shell' ]
    if as_root:
        adb_command.extend([ 'su', '-c' ])

    echo_command = '"echo {} > {}"'.format(datetime.now(), remote_file_path)
    adb_command.append(echo_command)

    adb_call(*adb_command, device = device_id)


def get_device_file_timestamp(remote_file_path, device_id, as_root = False):
    '''
    Get the integer timestamp value of a file from a given device.  Optionally, this
    command can be run as super user, or 'as_root', which is disabled by default.
    '''
    timestamp_string = ''

    ls_status, _ = adb_ls(args = [ '-l' ], path = remote_file_path, device_id = device_id, as_root = as_root)
    if ls_status:

        adb_command = [ 'shell' ]
        if as_root:
            adb_command.extend([ 'su', '-c' ])

        cat_command = '"cat {}"'.format(remote_file_path)
        adb_command.append(cat_command)

        file_contents = adb_call(*adb_command, device = device_id)
        if file_contents:
            timestamp_string = file_contents.strip()

    if timestamp_string:
        for fmt in ('%Y-%m-%d %H:%M:%S', '%Y-%m-%d %H:%M:%S.%f'):
            try:
                target_time = time.mktime(time.strptime(timestamp_string, fmt))
                break
            except:
                target_time = 0

        Logs.debug('android_deploy: {} time is {}'.format(remote_file_path, target_time))
        return target_time

    return 0


def auto_detect_device_storage_path(device_id, log_warnings = False):
    '''
    Uses the device's environment variable "EXTERNAL_STORAGE" to determine the correct
    path to public storage that has write permissions.  If at any point does the env var
    validation fail, fallback to checking known possible paths to external storage.
    '''
    def _log_debug(message):
        Logs.debug('android_deploy: {}'.format(message))

    def _log_warn(message):
        Logs.warn('[WARN] {}'.format(message))

    log_func = _log_warn if log_warnings else _log_debug

    def _check_known_paths():
        external_storage_paths = [
            '/sdcard/',
            '/storage/emulated/0/',
            '/storage/emulated/legacy/',
            '/storage/sdcard0/',
            '/storage/self/primary/',
        ]
        _log_debug('Falling back to search list of known external storage paths for device {}.'.format(device_id))

        for path in external_storage_paths:
            _log_debug('Searching {}'.format(path))
            status, _ = adb_ls(path, device_id)
            if status:
                return path[:-1]
        else:
            log_func('Failed to detect a valid path with correct permissions for device {}'.format(device_id))
            return ''

    external_storage = adb_call('shell', '"set | grep EXTERNAL_STORAGE"', device = device_id)
    if not external_storage:
        _log_debug('Call to get the EXTERNAL_STORAGE environment variable from device {} failed.'.format(device_id))
        return _check_known_paths()

    storage_path = external_storage.split('=')
    if len(storage_path) != 2:
        _log_debug('Received bad data while attempting extract the EXTERNAL_STORAGE environment variable from device {}.'.format(device_id))
        return _check_known_paths()

    var_path = storage_path[1].strip()
    status, _ = adb_ls(var_path, device_id)
    if status:
        return var_path

    _log_debug('The path specified in EXTERNAL_STORAGE seems to have permission issues, attempting to resolve with realpath for device {}.'.format(device_id))

    real_path = adb_call('shell', 'realpath', var_path, device = device_id)
    if not real_path:
        _log_debug('Something happened while attempting to resolve the path from the EXTERNAL_STORAGE environment variable for device {}.'.format(device_id))
        return _check_known_paths()

    real_path = real_path.strip()
    status, _ = adb_ls(real_path, device_id)
    if status:
        return real_path

    _log_debug('Unable to validate the resolved EXTERNAL_STORAGE environment variable path for device {}.'.format(device_id))
    return _check_known_paths()


def construct_assets_path_for_game_project(ctx, game_project):
    '''
    Generates the relative path from the root of public storage to the application's specific data folder
    '''
    return 'Android/data/{}/files'.format(ctx.get_android_package_name(game_project))


def build_shader_paks(ctx, game, assets_type, layout_node):

    shaders_pak_dir = generate_shaders_pak(ctx, game, assets_type, 'GLES3')

    shader_paks = shaders_pak_dir.ant_glob('*.pak')
    if not shader_paks:
        ctx.fatal('[ERROR] No shader pak files were found after running the pak_shaders command')

    # copy the shader paks into the layout directory
    shader_pak_dest = layout_node.make_node(game.lower())
    for pak in shader_paks:
        dest_node = shader_pak_dest.make_node(pak.name)
        dest_node.delete()

        Logs.debug('android_deploy: Copying {} => {}'.format(pak.relpath(), dest_node.relpath()))
        shutil.copy2(pak.abspath(), dest_node.abspath())


def pack_assets_in_apk(ctx, executable_name, layout_node):
    class command_buffer:
        def __init__(self, base_command_args):
            self._args_master = base_command_args
            self._base_len = len(' '.join(base_command_args))

            self.args = self._args_master[:]
            self.len = self._base_len

        def flush(self):
            if len(self.args) > len(self._args_master):
                command = ' '.join(self.args)
                Logs.debug('android_deploy: Running command - {}'.format(command))
                call(command, shell = True)

                self.args = self._args_master[:]
                self.len = self._base_len

    android_cache = get_android_cache_node(ctx)

    # create a copy of the existing bare bones APK for the assets
    variant = getattr(ctx.__class__, 'variant', None)
    if not variant:
        (platform, configuration) = ctx.get_platform_and_configuration()
        variant = '{}_{}'.format(platform, configuration)

    raw_apk_path = os.path.join(ctx.get_bintemp_folder_node().name, variant, ctx.get_android_project_relative_path(), executable_name, 'apk')
    barebones_apk_path = '{}/{}_unaligned_unsigned.apk'.format(raw_apk_path, executable_name)

    if not os.path.exists(barebones_apk_path):
        ctx.fatal('[ERROR] Unable to find the barebones APK in path {}.  Run the build command for {} again to generate it.'.format(barebones_apk_path, variant))

    apk_cache_node = android_cache.make_node(['apk', variant])
    apk_cache_node.mkdir()

    raw_apk_with_asset_node = apk_cache_node.make_node('{}_unaligned_unsigned{}.apk'.format(executable_name, APK_WITH_ASSETS_SUFFIX))

    shutil.copy2(barebones_apk_path, raw_apk_with_asset_node.abspath())

    # We need to make the 'assets' junction in order to generate the correct pathing structure when adding
    # files to an existing APK
    asset_dir = 'assets'
    asset_junction = android_cache.make_node(asset_dir)

    if os.path.exists(asset_junction.abspath()):
        remove_junction(asset_junction.abspath())

    try:
        Logs.debug('android_deploy: Creating assets junction "{}" ==> "{}"'.format(layout_node.abspath(), asset_junction.abspath()))
        junction_directory(layout_node.abspath(), asset_junction.abspath())
    except:
        ctx.fatal("[ERROR] Could not create junction for asset folder {}".format(layout_node.abspath()))

    # add the assets to the APK
    command = command_buffer([
        '"{}"'.format(ctx.env['AAPT']),
        'add',
        '"{}"'.format(raw_apk_with_asset_node.abspath())
    ])
    command_len_max = get_command_line_limit()

    with push_dir(android_cache.abspath()):
        ctx.user_message('Packing assets into the APK...')
        Logs.debug('android_deploy: -> from {}'.format(os.getcwd()))

        assets = asset_junction.ant_glob('**/*')
        for asset in assets:
            file_path = asset.path_from(android_cache)

            file_path = '"{}"'.format(file_path.replace('\\', '/'))

            path_len = len(file_path) + 1 # 1 for the space
            if (command.len + path_len) >= command_len_max:
                command.flush()

            command.len += path_len
            command.args.append(file_path)

        # flush the command buffer one more time
        command.flush()

    return apk_cache_node, raw_apk_with_asset_node


class adb_copy_output(Task):
    '''
    Class to handle copying of a single file in the layout to the android
    device.
    '''

    def __init__(self, *k, **kw):
        Task.__init__(self, *k, **kw)
        self.device = ''
        self.target = ''

    def set_device(self, device):
        '''Sets the android device (serial number from adb devices command) to copy the file to'''
        self.device = device

    def set_target(self, target):
        '''Sets the target file directory (absolute path) and file name on the device'''
        self.target = target

    def run(self):
        # Embed quotes in src/target so that we can correctly handle spaces
        src = '"{}"'.format(self.inputs[0].abspath())
        tgt = '"{}"'.format(self.target)

        Logs.debug('adb_copy_output: performing copy - {} to {} on device {}'.format(src, tgt, self.device))
        adb_call('push', src, tgt, device = self.device)

        return 0

    def runnable_status(self):
        if Task.runnable_status(self) == ASK_LATER:
            return ASK_LATER

        return RUN_ME


@taskgen_method
def adb_copy_task(self, android_device, src_node, output_target):
    '''
    Create a adb_copy_output task to copy the src_node to the ouput_target
    on the specified device. output_target is the absolute path and file name
    of the target file.
    '''
    copy_task = self.create_task('adb_copy_output', src_node)
    copy_task.set_device(android_device)
    copy_task.set_target(output_target)


###############################################################################
class DeployAndroidContext(Build.BuildContext):
    fun = 'deploy'


    def __init__(self, **kw):
        super(DeployAndroidContext, self).__init__(**kw)

        sdk_root = self.get_env_file_var('LY_ANDROID_SDK', required = True)

        global ADB_QUOTED_PATH
        ADB_QUOTED_PATH = '"{}"'.format(os.path.join(sdk_root, 'platform-tools', 'adb'))


    def get_asset_deploy_mode(self):
        if not hasattr(self, 'asset_deploy_mode'):
            asset_deploy_mode = self.options.deploy_android_asset_mode.lower()

            paks_required = self.env['CONFIGURATION'] in ('release', 'performance')

            if self.options.from_android_studio:
                # force a mode where assets are not packed into the APK since android studio handles the APK generation
                asset_deploy_mode = (ASSET_DEPLOY_PAKS if paks_required else ASSET_DEPLOY_LOOSE)

            elif paks_required:
                # force release mode to use the project's settings
                asset_deploy_mode = ASSET_DEPLOY_PROJECT_SETTINGS

            if asset_deploy_mode not in ASSET_DEPLOY_MODES:
                bld.fatal('[ERROR] Unable to determine the asset deployment mode.  Valid options for --deploy-android-asset-mode are limited to: {}'.format(ASSET_DEPLOY_MODES))

            self.asset_deploy_mode = asset_deploy_mode
            setattr(self.options, 'deploy_android_asset_mode', asset_deploy_mode)

        return self.asset_deploy_mode


    def use_vfs(self):
        if not hasattr(self, 'cached_use_vfs'):
            self.cached_use_vfs = (self.get_bootstrap_vfs() == '1')
        return self.cached_use_vfs


    def use_obb(self):
        if not hasattr(self, 'cached_use_obb'):
            game = self.get_bootstrap_game_folder()

            use_main_obb = (self.get_android_use_main_obb(game).lower() == 'true')
            use_patch_obb = (self.get_android_use_patch_obb(game).lower() == 'true')

            self.cached_use_obb = (use_main_obb or use_patch_obb)

        return self.cached_use_obb

    def get_asset_cache_path(self):
        if not hasattr(self, 'asset_cache_path'):
            game = self.get_bootstrap_game_folder()
            assets = self.get_bootstrap_assets('android')
            self.asset_cache_path = os.path.join('Cache', game, assets)

        return self.asset_cache_path

    def get_asset_cache(self):
        if not hasattr(self, 'asset_cache'):
            self.asset_cache = self.path.find_dir(self.get_asset_cache_path())

        return self.asset_cache


    def get_layout_node(self):
        if not hasattr(self, 'android_layout_node'):
            game = self.get_bootstrap_game_folder()
            asset_deploy_mode = self.get_asset_deploy_mode()

            if asset_deploy_mode == ASSET_DEPLOY_LOOSE:
                self.android_layout_node = self.get_asset_cache()

            elif asset_deploy_mode == ASSET_DEPLOY_PAKS:
                paks_cache = '{}_paks'.format(self.get_asset_cache_path())
                self.android_layout_node = self.path.make_node(paks_cache)

            elif asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS:
                cache_folder = '{}_{}'.format(self.get_asset_cache_path(), 'obb' if self.use_obb() else 'paks')
                self.android_layout_node = self.path.make_node(cache_folder)

        # just in case get_layout_node is called before deploy_android_asset_mode has been validated, which
        # could mean android_layout_node never getting set
        try:
            return self.android_layout_node
        except:
            self.fatal('[ERROR] Unable to determine the asset layout node for Android.')


    def user_message(self, message):
        Logs.pprint('CYAN', '[INFO] {}'.format(message))


    def log_error(self, message):
        if self.options.from_editor_deploy or self.options.from_android_studio:
            self.fatal(message)
        else:
            Logs.error(message)


# create a deployment context for each build variant
for target in ['android_armv7_clang', 'android_armv8_clang']:
    for configuration in get_supported_configurations(target):
        build_variant = '{}_{}'.format(target, configuration)

        package_context = type(
            '{}_package_context'.format(build_variant),
            (DeployAndroidContext,),
            {
                'cmd' : 'deploy_{}'.format(build_variant),
                'variant' : build_variant,
                'features' : ['deploy_android_prepare']
            })

        deploy_context = type(
            '{}_deploy_context'.format(build_variant),
            (DeployAndroidContext,),
            {
                'cmd' : 'deploy_devices_{}'.format(build_variant),
                'variant' : build_variant,
                'features' : ['deploy_android_devices']
            })


@taskgen_method
@feature('deploy_android_prepare')
def prepare_to_deploy_android(tsk_gen):
    '''
    Prepare the deploy process by generating the necessary asset layout
    directories, pak / obb files and packing assets in the APK if necessary.
    '''

    bld = tsk_gen.bld
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    # handle a few non-fatal early out cases
    if not bld.is_android_platform(platform) or not bld.is_option_true('deploy_android'):
        bld.user_message('Skipping Android Deployment...')
        return

    game = bld.get_bootstrap_game_folder()
    if game not in bld.get_enabled_game_project_list():
        Logs.warn('[WARN] The game project specified in bootstrap.cfg - {} - is not in the '
                    'enabled game projects list.  Skipping Android deployment...'.format(game))
        return

    # determine the selected asset deploy mode
    asset_deploy_mode = bld.get_asset_deploy_mode()
    Logs.debug('android_deploy: Using asset mode - {}'.format(asset_deploy_mode))

    assets_platform = bld.get_bootstrap_assets('android')

    if bld.get_asset_cache() is None:
        assets_dir = bld.get_asset_cache_path()
        output_folders = [ folder.name for folder in bld.get_standard_host_output_folders() ]

        bld.log_error('[ERROR] There is no asset cache to read from at "{}". Please run AssetProcessor or '
                      'AssetProcessorBatch from {} with "{}" assets enabled in the '
                      'AssetProcessorPlatformConfig.ini'.format(assets_dir, '/'.join(output_folders), assets_platform))
        return

    # handle the asset pre-processing
    if (asset_deploy_mode == ASSET_DEPLOY_PAKS) or (asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS):
        if bld.use_vfs():
            bld.fatal('[ERROR] Cannot use VFS when the --deploy-android-asset-mode is set to "{}".  Please set remote_filesystem=0 in bootstrap.cfg'.format(asset_deploy_mode))

        asset_cache = bld.get_asset_cache()
        layout_node = bld.get_layout_node()

        # generate the pak/obb files
        is_obb = (asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS) and bld.use_obb()
        rc_job_file = bld.get_android_rc_obb_job(game) if is_obb else bld.get_android_rc_pak_job(game)
        rc_job = os.path.join('Bin64', 'rc', rc_job_file)

        bld.user_message('Generating the necessary pak files...')
        run_rc_job(bld, game, rc_job, assets_platform, asset_cache.relpath(), layout_node.relpath(), is_obb)

        # handles the shaders
        if asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS:

            shadercachestartup_pak = '{}/shadercachestartup.pak'.format(game).lower()
            shaderscache_pak = '{}/shadercache.pak'.format(game).lower()

            bld.user_message('Generating the shader pak files...')
            build_shader_paks(bld, game, assets_platform, layout_node)

            # get the keystore passwords
            set_key_and_store_pass(bld)

            # generate the new apk with assets in it
            executable_name = bld.get_executable_name(game)
            apk_cache_node, raw_apk_with_asset = pack_assets_in_apk(bld, executable_name, layout_node)

            # sign and align the apk
            final_apk_out = bld.get_output_folders(platform, configuration)[0]
            tsk_gen.sign_and_align_apk(
                executable_name, # base_apk_name
                raw_apk_with_asset, # raw_apk
                apk_cache_node, # intermediate_folder
                final_apk_out, # final_output
                APK_WITH_ASSETS_SUFFIX # suffix
            )

    adb_call('start-server')

    if get_list_of_android_devices():
        Options.commands.append('deploy_devices_' + platform + '_' + configuration)

    else:
        if bld.options.from_editor_deploy:
            bld.fatal('[ERROR] No Android devices detected, unable to deploy')
        else:
            Logs.warn('[WARN] No Android devices detected, skipping deployment...')
        adb_call('kill-server')


@taskgen_method
@feature('deploy_android_devices')
def deploy_to_devices(tsk_gen):
    '''
    Installs the project APK and copies the layout directory to all the
    android devices that are connected to the host.
    '''
    def should_copy_file(src_file_node, target_time):
        should_copy = False
        try:
            stat_src = os.stat(src_file_node.abspath())
            should_copy = stat_src.st_mtime >= target_time
        except OSError:
            pass

        return should_copy

    bld = tsk_gen.bld
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    # ensure the adb server is running
    if adb_call('start-server') is None:
        bld.log_error('[ERROR] Failed to start adb server, unable to perform the deploy')
        return

    game = bld.get_bootstrap_game_folder()
    executable_name = bld.get_executable_name(game)

    asset_deploy_mode = bld.get_asset_deploy_mode()

    # create the path to the APK
    suffix = ''
    if asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS:
        suffix = APK_WITH_ASSETS_SUFFIX

    output_folder = bld.get_output_folders(platform, configuration)[0]
    apk_name = '{}/{}{}.apk'.format(output_folder.abspath(), executable_name, suffix)

    layout_node = bld.get_layout_node()

    do_clean = bld.is_option_true('deploy_android_clean_device')
    deploy_executable = bld.is_option_true('deploy_android_executable') and not bld.options.from_android_studio

    if bld.use_vfs():
        if asset_deploy_mode == ASSET_DEPLOY_LOOSE:
            deploy_assets = True
        else:
            # this is already checked in the prepare stage, but just to be safe...
            bld.fatal('[ERROR] Cannot use VFS when the --deploy-android-asset-mode is set to "{}".  Please set remote_filesystem=0 in bootstrap.cfg'.format(asset_deploy_mode))
    else:
        deploy_assets = asset_deploy_mode in (ASSET_DEPLOY_LOOSE, ASSET_DEPLOY_PAKS)

    # no sense in pushing the assets if we are cleaning the device and not reinstalling the APK from normal command line
    if do_clean and not deploy_executable and not bld.options.from_android_studio:
        deploy_assets = False

    Logs.debug('android_deploy: deploy options: do_clean {}, deploy_exec {}, deploy_assets {}'.format(do_clean, deploy_executable, deploy_assets))

    if deploy_executable and not os.path.exists(apk_name):
        bld.fatal('[ERROR] Could not find the Android executable (APK) in path - {} - necessary for deployment.  Run the build command for {}_{} to generate it'.format(apk_name, platform, configuration))
        return


    deploy_libs = bld.options.deploy_android_attempt_libs_only and not (do_clean or bld.options.from_editor_deploy or bld.options.from_android_studio) and not (asset_deploy_mode == ASSET_DEPLOY_PROJECT_SETTINGS)
    Logs.debug('android_deploy: The option to attempt library only deploy is %s', 'ENABLED' if deploy_libs else 'DISABLED')

    variant = '{}_{}'.format(platform, configuration)
    apk_builder_path = os.path.join( variant, bld.get_android_project_relative_path(), executable_name, 'builder' )
    apk_builder_node = bld.get_bintemp_folder_node().make_node(apk_builder_path)

    abi_func = getattr(bld, 'get_%s_target_abi' % platform, None)
    lib_paths = ['lib'] + [ abi_func() ] if abi_func else [] # since we don't support 'fat' apks it's ok to not have the abi specifier but it's still preferred
    stripped_libs_node = apk_builder_node.make_node(lib_paths)

    game_package = bld.get_android_package_name(game)
    device_install_path = '/data/data/{}'.format(game_package)

    if deploy_libs:
        apk_stat = os.stat(apk_name)
        apk_size = apk_stat.st_size


    relative_assets_path = construct_assets_path_for_game_project(bld, game)

    # This is the name of a file that we will use as a marker/timestamp. We
    # will get the timestamp of the file off the device and compare that with
    # asset files on the host machine to determine if the host machine asset
    # file is newer than what the device has, and if so copy it to the device.
    timestamp_file_name = 'deploy.timestamp'

    connected_devices = get_list_of_android_devices()
    target_devices = []

    device_filter = bld.options.deploy_android_device_filter
    if not device_filter:
        target_devices = connected_devices

    else:
        device_list = device_filter.split(',')
        for device_id in device_list:
            device_id = device_id.strip()
            if device_id not in connected_devices:
                Logs.warn('[WARN] Android device ID - {} - in device filter not detected as connected'.format(device_id))
            else:
                target_devices.append(device_id)

    deploy_count = 0
    for android_device in target_devices:
        bld.user_message('Starting to deploy to android device ' + android_device)

        storage_path = auto_detect_device_storage_path(android_device, log_warnings = True)
        if not storage_path:
            continue

        output_target = '{}/{}'.format(storage_path, relative_assets_path)
        device_timestamp_file = '{}/{}'.format(output_target, timestamp_file_name)

        if do_clean:
            bld.user_message('Cleaning target before deployment...')

            adb_call('shell', 'rm', '-rf', output_target, device = android_device)
            bld.user_message('Target Cleaned...')

            package_name = bld.get_android_package_name(game)
            if len(package_name) != 0:
                bld.user_message('Uninstalling package ' + package_name)
                adb_call('uninstall', package_name, device = android_device)

        ################################
        if deploy_libs:
            access_type = get_device_access_type(android_device)
            if access_type == ACCESS_NORMAL:
                Logs.warn('[WARN] android_deploy: Unable to perform the library only copy on device {}'.format(android_device))

            elif access_type in (ACCESS_ROOT_ADBD, ACCESS_SHELL_SU):
                device_file_staging_path = '{}/LY_Staging'.format(storage_path)
                device_lib_timestamp_file = '{}/files/{}'.format(device_install_path, timestamp_file_name)

                def _adb_push(source_node, dest, device_id):
                    adb_call('push', '"{}"'.format(source_node.abspath()), dest, device = device_id)

                def _adb_shell(source_node, dest, device_id):
                    temp_dest = '{}/{}'.format(device_file_staging_path, source_node.name)
                    adb_call('push', '"{}"'.format(source_node.abspath()), temp_dest, device = device_id)
                    adb_call('shell', 'su', '-c', 'cp', temp_dest, dest, device = device_id)

                if access_type == ACCESS_ROOT_ADBD:
                    adb_root_push_func = _adb_push

                elif access_type == ACCESS_SHELL_SU:
                    adb_root_push_func = _adb_shell
                    adb_call('shell', 'mkdir', device_file_staging_path)

                install_check = adb_call('shell', '"pm list packages | grep {}"'.format(game_package), device = android_device)
                if install_check:
                    target_time = get_device_file_timestamp(device_lib_timestamp_file, android_device, True)

                    # cases for early out in favor of re-installing the APK:
                    #       If target_time is zero, the file wasn't found which would indicate we haven't attempt to push just the libs before
                    #       The dalvik executable is newer than the last time we deployed to this device
                    if target_time == 0 or should_copy_file(apk_builder_node.make_node('classes.dex'), target_time):
                        bld.user_message('A new APK needs to be installed instead for device {}'.format(android_device))

                    # otherwise attempt to copy the libs directly
                    else:
                        bld.user_message('Scanning which libraries need to be copied...')

                        libs_to_add = []
                        total_libs_size = 0
                        fallback_to_apk = False

                        libs = stripped_libs_node.ant_glob('**/*.so')
                        for lib in libs:

                            if should_copy_file(lib, target_time):
                                lib_stat = os.stat(lib.abspath())
                                total_libs_size += lib_stat.st_size
                                libs_to_add.append(lib)

                                if total_libs_size >= apk_size:
                                    bld.user_message('Too many libriares changed, falling back to installing a new APK on {}'.format(android_device))
                                    fallback_to_apk = True
                                    break

                        if not fallback_to_apk:
                            for lib in libs_to_add:
                                final_target_dir = '{}/lib/{}'.format(device_install_path, lib.name)

                                adb_root_push_func(lib, final_target_dir, android_device)

                                adb_call('shell', 'su', '-c', 'chown', LIB_OWNER_GROUP, final_target_dir, device = android_device)
                                adb_call('shell', 'su', '-c', 'chmod', LIB_FILE_PERMISSIONS, final_target_dir, device = android_device)

                            deploy_executable = False

                update_device_file_timestamp(device_lib_timestamp_file, android_device, as_root = True)

                # clean up the staging directory
                if access_type == ACCESS_SHELL_SU:
                    adb_call('shell', 'rm', '-rf', device_file_staging_path)

        ################################
        if deploy_executable:
            install_options = getattr(Options.options, 'deploy_android_install_options')
            replace_package = ''
            if bld.is_option_true('deploy_android_replace_apk'):
                replace_package = '-r'

            bld.user_message('Installing ' + apk_name)
            install_result = adb_call('install', install_options, replace_package, '"{}"'.format(apk_name), device = android_device)
            if not install_result or 'success' not in install_result.lower():
                Logs.warn('[WARN] android deploy: failed to install APK on device %s.' % android_device)
                if install_result:
                    # The error msg is the last non empty line of the output.
                    error_msg = next(error for error in reversed(install_result.splitlines()) if error)
                    Logs.warn('[WARN] android deploy: %s' % error_msg)

                continue

        if deploy_assets:

            if bld.use_vfs():
                files_to_transfer = [
                    'bootstrap.cfg',
                    '{}/config/game.xml'.format(game).lower()
                ]

                for file in files_to_transfer:
                    local_node = layout_node.find_resource(file)
                    if not local_node:
                        bld.fatal('[ERROR] Failed to locate file {} in path {}'.format(os.path.normpath(file), layout_node.abspath()))

                    target_file = '{}/{}'.format(output_target, file)
                    tsk_gen.adb_copy_task(android_device, local_node, target_file)

            else:
                target_time = get_device_file_timestamp(device_timestamp_file, android_device)

                if do_clean or target_time == 0:
                    bld.user_message('Copying all assets to the device {}. This may take some time...'.format(android_device))

                    # There is a chance that if someone ran VFS before this deploy on an empty directory the output_target directory will have
                    # already existed when we do the push command. In this case if we execute adb push command it will create an ES3 directory
                    # and put everything there, causing the deploy to be 'successful' but the game will crash as it won't be able to find the
                    # assets. Since we detected a "clean" build, wipe out the output_target folder if it exists first then do the push and
                    # everything will be just fine.
                    ls_status, _ = adb_ls(output_target, android_device)
                    if ls_status:
                        adb_call('shell', 'rm', '-rf', output_target)

                    push_status = adb_call('push', '"{}"'.format(layout_node.abspath()), output_target, device = android_device)
                    if not push_status:
                        # Clean up any files we may have pushed to make the next run success rate better
                        adb_call('shell', 'rm', '-rf', output_target, device = android_device)
                        bld.fatal('[ERROR] The ABD command to push all the files to the device failed.')
                        continue

                else:
                    layout_files = layout_node.ant_glob('**/*')
                    bld.user_message('Scanning {} files to determine which ones need to be copied...'.format(len(layout_files)))
                    for src_file in layout_files:
                        # Faster to check if we should copy now rather than in the copy_task
                        if should_copy_file(src_file, target_time):
                            final_target_dir = '{}/{}'.format(output_target, string.replace(src_file.path_from(layout_node), '\\', '/'))
                            tsk_gen.adb_copy_task(android_device, src_file, final_target_dir)

                update_device_file_timestamp(device_timestamp_file, android_device)

        deploy_count = deploy_count + 1

    if not deploy_count:
        bld.fatal('[ERROR] Failed to deploy the build to any connected devices.')

