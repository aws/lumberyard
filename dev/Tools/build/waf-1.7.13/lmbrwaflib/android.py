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
import os, sys, random, time, re, stat, string, imghdr
import atexit, shutil, threading, collections, hashlib, subprocess
import xml.etree.ElementTree as ET

from subprocess import call, check_output

from cry_utils import append_to_unique_list
from third_party import is_third_party_uselib_configured

from waflib import Context, TaskGen, Build, Utils, Node, Logs, Options, Errors
from waflib.Build import POST_LAZY, POST_AT_ONCE
from waflib.Configure import conf
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waflib.TaskGen import feature, extension, before, before_method, after, after_method, taskgen_method

from waflib.Tools import ccroot
ccroot.USELIB_VARS['android'] = set([ 'AAPT', 'APPT_RESOURCES', 'APPT_INCLUDES', 'APP_PACKAGE_FLAGS' ])


################################################################
#                     Defaults                                 #
BUILDER_DIR = 'Code/Launcher/AndroidLauncher/ProjectBuilder'
BUILDER_FILES = 'android_builder.json'
ANDROID_LIBRARY_FILES = 'android_libraries.json'

RESOLUTION_MESSAGE = 'Please re-run Setup Assistant with "Compile For Android" enabled and run the configure command again.'

RESOLUTION_SETTINGS = ( 'mdpi', 'hdpi', 'xhdpi', 'xxhdpi', 'xxxhdpi' )

LATEST_KEYWORD = 'latest'

# these are the default names for application icons and splash images
APP_ICON_NAME = 'app_icon.png'
APP_SPLASH_NAME = 'app_splash.png'

# supported api versions
SUPPORTED_APIS = [
    'android-19',
    'android-21',
    'android-22',
    'android-23',
    'android-24'
]

# the default file permissions after copies are made.  511 => 'chmod 777 <file>'
FILE_PERMISSIONS = 511

AUTO_GEN_HEADER_PYTHON = r'''
################################################################
# This file was automatically created by WAF
# WARNING! All modifications will be lost!
################################################################

'''
#                                                              #
################################################################


################################################################
"""
Parts of the Android build process require the ability to create directory junctions/symlinks to make sure some assets are properly
included in the build while maintaining as small of a memory footprint as possible (don't want to make copies).  Since we only care
about directories, we don't need the full power of os.symlink (doesn't work on windows anyway and writing one would require either
admin privileges or running something such as a VB script to create a shortcut to bypass the admin issue; neither of those options
are desirable).  The following functions are to make it explicitly clear we only care about directory links.
"""
def junction_directory(source, link_name):
    if not os.path.isdir(source):
        Logs.error("[ERROR] Attempting to make a junction to a file, which is not supported.  Unexpected behaviour may result.")
        return

    if Utils.unversioned_sys_platform() == "win32":
        cleaned_source_name = '"' + source.replace('/', '\\') + '"'
        cleaned_link_name = '"' + link_name.replace('/', '\\') + '"'

        # mklink generaully requires admin privileges, however directory junctions don't.
        # subprocess.check_call will auto raise.
        subprocess.check_call('mklink /D /J %s %s' % (cleaned_link_name, cleaned_source_name), shell=True)
    else:
        os.symlink(source, link_name)


def is_directory_junction(path):
    if not os.path.exists(path):
        return False

    if not os.path.isdir(path):
        Logs.error("[ERROR] Attempting to querry if a file is a junction, which is not supported.  Unexpected behaviour may result.")
        return False

    if Utils.unversioned_sys_platform() == "win32":
        import ctypes, ctypes.wintypes
        FILE_ATTRIBUTE_REPARSE_POINT = 0x0400

        cleaned_path = '"' + path.replace('/', '\\') + '"'

        try:
            csl = ctypes.windll.kernel32.GetFileAttributesW
            csl.argtypes = [ctypes.c_wchar_p]
            csl.restype = ctypes.wintypes.DWORD

            attributes = csl(cleaned_path)
        except:
            attributes = 0
            pass

        return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT
    else:
        return os.path.islink(path)


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


@feature('find_android_api_libs')
@before_method('propagate_uselib_vars')
def find_api_lib(self):
    """
    Goes through the list of libraries included by the 'uselib' keyword and search if there's a better version of the library for the
    API level being used. If one is found, the new version is used instead.
    The api specific version must have the same name as the original library + "_android-XX".
    For example, if we are including a library called "foo", and we also have defined "foo_android-21", when compiling with API 23,
    "foo" will be replaced with "foo_android-21". We do this for supporting multiple versions of a library depending on which API
    level the user is using.
    """

    def find_api_helper(lib_list, api_list):
        if not lib_list or not api_list:
            return

        for idx, lib in enumerate(lib_list):
            for api_level in api_list:
                lib_name = (lib + '_' + api_level).upper()
                if is_third_party_uselib_configured(self.bld, lib_name):
                    lib_list[idx] = lib_name
                    break

    if 'android' not in self.bld.env['PLATFORM']:
        return

    uselib_keys = getattr(self, 'uselib', None)
    use_keys = getattr(self, 'use', None)
    if uselib_keys or use_keys:
        # sort the list of supported apis and search if there's a better version depending on the Android NDK platform version being used.
        api_list = sorted(SUPPORTED_APIS)
        ndk_platform = self.bld.get_android_ndk_platform()
        try:
            index = api_list.index(ndk_platform)
        except:
            self.bld.fatal('[ERROR] Unsupported Android NDK platform version %s' % ndk_platform)
            return

        # we can only use libs built with api levels lower or equal to the one being used.
        api_list = api_list[:index + 1] # end index is exclusive, so we add 1
        # reverse the list so we use the lib with the higher api (if we find one).
        api_list.reverse()
        find_api_helper(uselib_keys, api_list)
        find_api_helper(use_keys, api_list)

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
        relative_path = os.path.join('Code', project, 'Resources', source_path)
        path_node = conf.path.make_node(relative_path)
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

    group.add_option('--android-toolchain', dest = 'android_toolchain', action = 'store', default = '', help = 'DEPRECATED: Android toolchain to use for building, valid options are gcc or clang')

    group.add_option('--android-sdk-version-override', dest = 'android_sdk_version_override', action = 'store', default = '', help = 'Override the Android SDK version used in the Java compilation.  Only works during configure.')
    group.add_option('--android-ndk-platform-override', dest = 'android_ndk_platform_override', action = 'store', default = '', help = 'Override the Android NDK platform version used in the native compilation.  Only works during configure.')

    group.add_option('--dev-store-pass', dest = 'dev_store_pass', action = 'store', default = 'Lumberyard', help = 'The store password for the development keystore')
    group.add_option('--dev-key-pass', dest = 'dev_key_pass', action = 'store', default = 'Lumberyard', help = 'The key password for the development keystore')

    group.add_option('--distro-store-pass', dest = 'distro_store_pass', action = 'store', default = '', help = 'The store password for the distribution keystore')
    group.add_option('--distro-key-pass', dest = 'distro_key_pass', action = 'store', default = '', help = 'The key password for the distribution keystore')

    group.add_option('--android-apk-path', dest = 'apk_path', action = 'store', default = '', help = 'Path to apk to deploy. If not specified the default build path will be used')

    group.add_option('--from-editor-deploy', dest = 'from_editor_deploy', action = 'store_true', default = False, help = 'Signals that the build is coming from the editor deployment tool')


################################################################
def configure(conf):

    env = conf.env

    # validate the stored sdk and ndk paths from SetupAssistant
    sdk_root = conf.get_env_file_var('LY_ANDROID_SDK', required = True)
    ndk_root = conf.get_env_file_var('LY_ANDROID_NDK', required = True)

    if not (sdk_root and ndk_root):
        missing_paths = []
        missing_paths += ['Android SDK'] if not sdk_root else []
        missing_paths += ['Android NDK'] if not ndk_root else []

        conf.fatal('[ERROR] Missing paths from Setup Assistant detected for: {}.  {}'.format(', '.join(missing_paths), RESOLUTION_MESSAGE))

    env['ANDROID_SDK_HOME'] = sdk_root
    env['ANDROID_NDK_HOME'] = ndk_root

    # get the revision of the NDK
    with open(os.path.join(ndk_root, 'source.properties')) as ndk_props_file:
        for line in ndk_props_file.readlines():
            tokens = line.split('=')
            trimed_tokens = [token.strip() for token in tokens]

            if 'Pkg.Revision' in trimed_tokens:
                ndk_rev = trimed_tokens[1]

    env['ANDROID_NDK_REV_FULL'] = ndk_rev

    ndk_rev_tokens = ndk_rev.split('.')
    env['ANDROID_NDK_REV_MAJOR'] = int(ndk_rev_tokens[0])
    env['ANDROID_NDK_REV_MINOR'] = int(ndk_rev_tokens[1])

    headers_type = 'unified platform' if conf.is_using_android_unified_headers() else 'platform specific'
    Logs.debug('android: Using {} headers with Android NDK revision {}.'.format(headers_type, ndk_rev))

    # validate the desired SDK version
    installed_sdk_versions = os.listdir(os.path.join(sdk_root, 'platforms'))
    valid_sdk_versions = [platorm for platorm in installed_sdk_versions if platorm in SUPPORTED_APIS]
    Logs.debug('android: Valid installed SDK versions are: {}'.format(valid_sdk_versions))

    sdk_version = Options.options.android_sdk_version_override
    if not sdk_version:
        sdk_version = conf.get_android_sdk_version()

    if sdk_version.lower() == LATEST_KEYWORD:
        if not valid_sdk_versions:
            conf.fatal('[ERROR] Unable to detect a valid Android SDK version installed in path {}.  '
                        'Please use the Android SDK Manager to download an appropriate SDK version and run the configure command again.\n'
                        '\t-> Supported APIs installed are: {}'.format(sdk_root, ', '.join(SUPPORTED_APIS)))

        valid_sdk_versions = sorted(valid_sdk_versions)
        sdk_version = valid_sdk_versions[-1]
        Logs.debug('android: Using the latest installed Android SDK version {}'.format(sdk_version))

    else:
        if sdk_version not in SUPPORTED_APIS:
            conf.fatal('[ERROR] Android SDK version - {} - is unsupported.  Please change SDK_VERSION in _WAF_/android/android_setting.json to a supported API and run the configure command again.\n'
                        '\t-> Supported APIs are: {}'.format(sdk_version, ', '.join(SUPPORTED_APIS)))

        if sdk_version not in valid_sdk_versions:
            conf.fatal('[ERROR] Failed to find Android SDK version - {} - installed in path {}.  '
                        'Please use the Android SDK Manager to download the appropriate SDK version or change SDK_VERSION in _WAF_/android/android_settings.json to a supported version installed and run the configure command again.\n'
                        '\t-> Supported APIs installed are: {}'.format(sdk_version, sdk_root, ', '.join(valid_sdk_versions)))

    env['ANDROID_SDK_VERSION'] = sdk_version
    env['ANDROID_SDK_VERSION_NUMBER'] = int(sdk_version.split('-')[1])

    # validate the desired NDK platform version
    ndk_platform = Options.options.android_ndk_platform_override
    if not ndk_platform:
        ndk_platform = conf.get_android_ndk_platform()

    ndk_sdk_match = False
    if not ndk_platform:
        Logs.debug('android: The Android NDK platform version has not been specified.  Auto-detecting from specified Android SDK version {}.'.format(sdk_version))
        ndk_platform = sdk_version
        ndk_sdk_match = True

    ndk_platforms = os.listdir(os.path.join(ndk_root, 'platforms'))
    valid_ndk_platforms = [platorm for platorm in ndk_platforms if platorm in SUPPORTED_APIS]
    Logs.debug('android: Valid NDK platforms for revision {} are: {}'.format(ndk_rev, valid_ndk_platforms))

    if ndk_platform not in valid_ndk_platforms:
        if ndk_sdk_match:
            # search the valid ndk platforms for one that is closest, but lower, than the desired sdk version
            sorted_valid_platforms = sorted(valid_ndk_platforms)
            for platform in sorted_valid_platforms:
                if platform <= sdk_version:
                    ndk_platform = platform
            Logs.debug('android: Closest Android NDK platform version detected from Android SDK version {} is {}'.format(sdk_version, ndk_platform))
        else:
            platform_list = ', '.join(valid_ndk_platforms)
            conf.fatal("[ERROR] Attempting to use a NDK platform - {} - that is either unsupported or doesn't have platform specific headers.  "
                        "Please set NDK_PLATFORM in _WAF_/android/android_settings.json to a valid platform or remove to auto-detect from SDK_VERSION and run the configure command again.\n"
                        "\t-> Valid platforms for NDK {} include: {}".format(ndk_platform, ndk_rev, platform_list))

    env['ANDROID_NDK_PLATFORM'] = ndk_platform
    env['ANDROID_NDK_PLATFORM_NUMBER'] = int(ndk_platform.split('-')[1])

    # final check is to make sure the ndk platform <= sdk version to ensure compatibilty
    if not (ndk_platform <= sdk_version):
        conf.fatal('[ERROR] The Android API specified in NDK_PLATFORM - {} - is newer than the API specified in SDK_VERSION - {}; this can lead to compatibilty issues.\n'
                    'Please update your _WAF_/android/android_settings.json to make sure NDK_PLATFORM <= SDK_VERSION and run the configure command again.'.format(ndk_platform, sdk_version))

    # validate the desired SDK build-tools version
    build_tools_version = conf.get_android_build_tools_version()

    build_tools_dir = os.path.join(sdk_root, 'build-tools')
    build_tools_dir_contents = os.listdir(build_tools_dir)
    installed_build_tools_versions = [ entry for entry in build_tools_dir_contents if entry.split('.')[0].isdigit() ]

    if build_tools_version.lower() == LATEST_KEYWORD:
        if not installed_build_tools_versions:
            conf.fatal('[ERROR] Unable to detect a valid Android SDK build-tools version installed in path {}.  '
                        'Please use the Android SDK Manager to download the appropriate build-tools version and run the configure command again.'.format(sdk_root))

        installed_build_tools_versions = sorted(installed_build_tools_versions)
        build_tools_version = installed_build_tools_versions[-1]
        Logs.debug('android: Using the latest installed Android SDK build tools version {}'.format(build_tools_version))

    elif build_tools_version not in installed_build_tools_versions:
        conf.fatal('[ERROR] Failed to find Android SDK build-tools version - {} - installed in path {}.  Please use Android SDK Manager to download the appropriate build-tools version '
                    'or change BUILD_TOOLS_VER in _WAF_/android/android_settings.json to either a version installed or "latest" and run the configure command again.\n'
                    '\t-> Installed build-tools version detected: {}'.format(build_tools_version, build_tools_dir, ', '.join(installed_build_tools_versions)))

    conf.env['ANDROID_BUILD_TOOLS_VER'] = build_tools_version


################################################################
@conf
def is_using_android_unified_headers(conf):
    """
    NDK r14 introduced unified headers which is to replace the old platform specific set
    of headers in previous versions of the NDK.  There is a bug in one of the headers
    (strerror_r isn't proper defined) in this version but fixed in NDK r14b.  Because of
    this we will make NDK r14b our min spec for using the unified headers.
    """
    env = conf.env

    ndk_rev_major = env['ANDROID_NDK_REV_MAJOR']
    ndk_rev_minor = env['ANDROID_NDK_REV_MINOR']

    return ((ndk_rev_major == 14 and ndk_rev_minor >= 1) or (ndk_rev_major >= 15))


################################################################
@conf
def load_android_toolchains(conf, search_paths, CC, CXX, AR, STRIP, **addition_toolchains):
    """
    Helper function for loading all the android toolchains
    """
    try:
        conf.find_program(CC, var = 'CC', path_list = search_paths, silent_output = True)
        conf.find_program(CXX, var = 'CXX', path_list = search_paths, silent_output = True)
        conf.find_program(AR, var = 'AR', path_list = search_paths, silent_output = True)

        # for debug symbol stripping
        conf.find_program(STRIP, var = 'STRIP', path_list = search_paths, silent_output = True)

        # optional linker override
        if 'LINK_CC' in addition_toolchains and 'LINK_CXX' in addition_toolchains:
            conf.find_program(addition_toolchains['LINK_CC'], var = 'LINK_CC', path_list = search_paths, silent_output = True)
            conf.find_program(addition_toolchains['LINK_CXX'], var = 'LINK_CXX', path_list = search_paths, silent_output = True)
        else:
            conf.env['LINK_CC'] = conf.env['CC']
            conf.env['LINK_CXX'] = conf.env['CXX']

        conf.env['LINK'] = conf.env['LINK_CC']

        # common cc settings
        conf.cc_load_tools()
        conf.cc_add_flags()

        # common cxx settings
        conf.cxx_load_tools()
        conf.cxx_add_flags()

        # common link settings
        conf.link_add_flags()
    except:
        Logs.error('[ERROR] Failed to find the Android NDK standalone toolchain(s) in search path %s' % search_paths)
        return False

    return True


################################################################
@conf
def load_android_tools(conf):
    """
    This function is intended to be called in all android compiler setttings/rules
    so the android sdk build tools are part of the environment
    """

    build_tools_version = conf.get_android_build_tools_version()
    build_tools_dir = os.path.join(conf.env['ANDROID_SDK_HOME'], 'build-tools')
    build_tools = os.path.join(build_tools_dir, build_tools_version)

    try:
        conf.find_program('aapt', var = 'AAPT', path_list = [ build_tools ], silent_output = True)
        conf.find_program('dx', var = 'DX', path_list = [ build_tools ], silent_output = True)
        conf.find_program('zipalign', var = 'ZIPALIGN', path_list = [ build_tools ], silent_output = True)
        conf.find_program('aidl', var = 'AIDL', path_list = [ build_tools ], silent_output = True)
    except:
        Logs.error('[ERROR] The desired Android SDK build-tools version - {} - appears to be incomplete.  Please use Android SDK Manager to validate the build-tools version installation '
                    'or change BUILD_TOOLS_VER in _WAF_/android/android_settings.json to either a version installed or "latest" and run the configure command again.\n'.format(build_tools_version))
        return False

    return True


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

        target_curr.chmod(FILE_PERMISSIONS)
        copied_files.append(target_curr.abspath())


################################################################
'''
Copy the libraries that need to be patched and do the patching of the files.
'''
def copy_and_patch_android_libraries(conf, source_node, android_root):
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
        dest_path = os.path.join(conf.Path(conf.get_android_patched_libraries_relative_path()), lib.name)
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
@conf
def create_and_add_android_launchers_to_build(conf):
    """
    This function will generate the bare minimum android project
    and include the new android launcher(s) in the build path.
    So no Android Studio gradle files will be generated.
    """
    android_root = conf.path.make_node(conf.get_android_project_relative_path())
    android_root.mkdir()

    source_node = conf.path.make_node(BUILDER_DIR)
    builder_file_src = source_node.make_node(BUILDER_FILES)
    builder_file_dest = conf.path.get_bld().make_node(BUILDER_DIR)

    if not os.path.exists(builder_file_src.abspath()):
        conf.fatal('[ERROR] Failed to find the Android project builder - %s - in path %s.  Verify file exists and run the configure command again.' % (BUILDER_FILES, BUILDER_DIR))
        return False

    created_directories = []
    for project in conf.get_enabled_game_project_list():
        # make sure the project has android options specified
        if conf.get_android_settings(project) == None:
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
            'ANDROID_PROJECT_NAME' : project,                                # internal facing name (library/asset dir name)

            'ANDROID_PROJECT_ACTIVITY' : activity_name,
            'ANDROID_LAUNCHER_NAME' : conf.get_executable_name(project),     # first native library to load from java

            'ANDROID_VERSION_NUMBER' : conf.get_android_version_number(project),
            'ANDROID_VERSION_NAME' : conf.get_android_version_name(project),

            'ANDROID_SCREEN_ORIENTATION' : conf.get_android_orientation(project),

            'ANDROID_APP_PUBLIC_KEY' : conf.get_android_app_public_key(project),
            'ANDROID_APP_OBFUSCATOR_SALT' : conf.get_android_app_obfuscator_salt(project),

            'ANDROID_USE_MAIN_OBB' : conf.get_android_use_main_obb(project),
            'ANDROID_USE_PATCH_OBB' : conf.get_android_use_patch_obb(project),
            'ANDROID_ENABLE_OBB_IN_DEV' : conf.get_android_enable_obb_in_dev(project),
            'ANDROID_ENABLE_KEEP_SCREEN_ON' : conf.get_android_enable_keep_screen_on(project),

            'ANDROID_MIN_SDK_VERSION' : conf.env['ANDROID_NDK_PLATFORM_NUMBER'],
        }

        # update the builder file with the correct pacakge name
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
        if icon_overrides is not None:
            mipmap_path_prefix = 'mipmap'

            # if a default icon is specified, then copy it into the generic mipmap folder
            default_icon = icon_overrides.get('default', None)

            if default_icon is not None:
                default_icon_source_node = construct_source_path(conf, project, default_icon)

                default_icon_target_dir = resource_node.make_node(mipmap_path_prefix)
                default_icon_target_dir.mkdir()

                dest_file = os.path.join(default_icon_target_dir.abspath(), APP_ICON_NAME)
                shutil.copyfile(default_icon_source_node, dest_file)
                os.chmod(dest_file, FILE_PERMISSIONS)
                copied_files.append(dest_file)

            else:
                Logs.debug('android: No default icon override specified for %s' % project)

            # process each of the resolution overrides
            for resolution in RESOLUTION_SETTINGS:
                target_directory = resource_node.make_node(mipmap_path_prefix + '-' + resolution)

                # get the current resolution icon override
                icon_source = icon_overrides.get(resolution, default_icon)
                if icon_source is default_icon:

                    # if both the resolution and the default are unspecified, warn the user but do nothing
                    if icon_source is None:
                        Logs.warn('[WARN] No icon override found for "%s".  Either supply one for "%s" or a "default" in the android_settings "icon" section of the project.json file for %s' % (resolution, resolution, project))

                    # if only the resoultion is unspecified, remove the resolution specific version from the project
                    else:
                        Logs.debug('android: Default icon being used for "%s" in %s' % (resolution, project))
                        remove_file_and_empty_directory(target_directory.abspath(), APP_ICON_NAME)

                    continue

                icon_source_node = construct_source_path(conf, project, icon_source)
                icon_target_node = target_directory.make_node(APP_ICON_NAME)

                shutil.copyfile(icon_source_node, icon_target_node.abspath())
                icon_target_node.chmod(FILE_PERMISSIONS)
                copied_files.append(icon_target_node.abspath())

        # resolve the application splash screen overrides
        splash_overrides = conf.get_android_app_splash_screens(project)
        if splash_overrides is not None:
            drawable_path_prefix = 'drawable-'

            for orientation in ('land', 'port'):
                orientation_path_prefix = drawable_path_prefix + orientation

                oriented_splashes = splash_overrides.get(orientation, {})

                # if a default splash image is specified for this orientation, then copy it into the generic drawable-<orientation> folder
                default_splash_img = oriented_splashes.get('default', None)

                if default_splash_img is not None:
                    default_splash_img_source_node = construct_source_path(conf, project, default_splash_img)

                    default_splash_img_target_dir = resource_node.make_node(orientation_path_prefix)
                    default_splash_img_target_dir.mkdir()

                    dest_file = os.path.join(default_splash_img_target_dir.abspath(), APP_SPLASH_NAME)
                    shutil.copyfile(default_splash_img_source_node, dest_file)
                    os.chmod(dest_file, FILE_PERMISSIONS)
                    copied_files.append(dest_file)

                else:
                    Logs.debug('android: No default splash screen override specified for "%s" orientation in %s' % (orientation, project))

                # process each of the resolution overrides
                for resolution in RESOLUTION_SETTINGS:
                    # The xxxhdpi resolution is only for application icons, its overkill to include them for drawables... for now
                    if resolution == 'xxxhdpi':
                        continue

                    target_directory = resource_node.make_node(orientation_path_prefix + '-' + resolution)

                    # get the current resolution splash image override
                    splash_img_source = oriented_splashes.get(resolution, default_splash_img)
                    if splash_img_source is default_splash_img:

                        # if both the resolution and the default are unspecified, warn the user but do nothing
                        if splash_img_source is None:
                            section = "%s-%s" % (orientation, resolution)
                            Logs.warn('[WARN] No splash screen override found for "%s".  Either supply one for "%s" or a "default" in the android_settings "splash_screen-%s" section of the project.json file for %s' % (section, resolution, orientation, project))

                        # if only the resoultion is unspecified, remove the resolution specific version from the project
                        else:
                            Logs.debug('android: Default icon being used for "%s-%s" in %s' % (orientation, resolution, project))
                            remove_file_and_empty_directory(target_directory.abspath(), APP_SPLASH_NAME)

                        continue

                    splash_img_source_node = construct_source_path(conf, project, splash_img_source)
                    splash_img_target_node = target_directory.make_node(APP_SPLASH_NAME)

                    shutil.copyfile(splash_img_source_node, splash_img_target_node.abspath())
                    splash_img_target_node.chmod(FILE_PERMISSIONS)
                    copied_files.append(splash_img_target_node.abspath())

        # additional optimization to only include the splash screens for the avaiable orientations allowed by the manifest
        requested_orientation = conf.get_android_orientation(project)

        if requested_orientation in ('landscape', 'reverseLandscape', 'sensorLandscape', 'userLandscape'):
            Logs.debug('android: Clearing the portrait assets from %s' % project)
            clear_splash_assets(resource_node, 'drawable-port')

        elif requested_orientation in ('portrait', 'reversePortrait', 'sensorPortrait', 'userPortrait'):
            Logs.debug('android: Clearing the landscape assets from %s' % project)
            clear_splash_assets(resource_node, 'drawable-land')

        # delete all files from the destination folder that were not copied by the script
        all_files = proj_root.ant_glob("**", excl=['wscript', 'build.gradle', 'assets_for_apk/*'])
        files_to_delete = [path for path in all_files if path.abspath() not in copied_files]

        for file in files_to_delete:
            file.delete()

    # add all the projects to the root wscript
    android_wscript = android_root.make_node('wscript')
    with open(android_wscript.abspath(), 'w') as wscript_file:
        w = wscript_file.write

        w(AUTO_GEN_HEADER_PYTHON)

        w('SUBFOLDERS = [\n')
        w('\t\'%s\'\n]\n\n' % '\',\n\t\''.join(created_directories))

        w('def build(bld):\n')
        w('\tvalid_subdirs = [x for x in SUBFOLDERS if bld.path.find_node("%s/wscript" % x)]\n')
        w('\tbld.recurse(valid_subdirs)\n')

    # Some Android SDK libraries have bugs, so we need to copy them locally and patch them.
    if not copy_and_patch_android_libraries(conf, source_node, android_root):
        return False

    return True


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
def collect_source_paths(android_task, src_path_tag):

    game_project = android_task.game_project
    bld = android_task.bld

    platform = bld.env['PLATFORM']
    config = bld.env['CONFIGURATION']

    search_tags = [
        'android_{}'.format(src_path_tag),
        'android_{}_{}'.format(config, src_path_tag),

        '{}_{}'.format(platform, src_path_tag),
        'android_{}_{}'.format(platform, config, src_path_tag),
    ]

    sourc_paths = []
    for group in bld.groups:
        for task_generator in group:
            if not isinstance(task_generator, TaskGen.task_gen):
                continue

            Logs.debug('android: Processing task %s' % task_generator.name)

            if not (getattr(task_generator, 'posted', None) and getattr(task_generator, 'link_task', None)):
                Logs.debug('android:  -> Task is NOT posted, Skipping...')
                continue

            project_name = getattr(task_generator, 'project_name', None)
            if not bld.is_module_for_game_project(task_generator.name, game_project, project_name):
                Logs.debug('android:  -> Task is NOT part of the game project, Skipping...')
                continue

            raw_paths = []
            for tag in search_tags:
                raw_paths += getattr(task_generator, tag, [])

            Logs.debug('android:  -> Raw Source Paths %s' % raw_paths)

            for path in raw_paths:
                if os.path.isabs(path):
                    path = bld.root.make_node(path)
                else:
                    path = task_generator.path.make_node(path)
                sourc_paths.append(path)

    return sourc_paths


################################################################
def get_resource_compiler_path(ctx):
    if Utils.unversioned_sys_platform() == "win32":
        paths = ['Bin64vc140', 'Bin64vc120', 'Bin64']
    else:
        paths = ['BinMac64', 'Bin64']
    rc_search_paths = [os.path.join(ctx.path.abspath(), path, 'rc') for path in paths]
    try:
        return ctx.find_program('rc', path_list = rc_search_paths, silent_output = True)
    except:
        Logs.warn('[WARN] Failed to find the Resource Compiler')
        return None


################################################################
################################################################
class strip_debug_symbols(Task):
    """
    Strips the debug symbols from a shared library
    """
    color = 'CYAN'
    run_str = "${STRIP} --strip-debug -o ${TGT} ${SRC}"
    vars = [ 'STRIP' ]

    def runnable_status(self):
        if super(strip_debug_symbols, self).runnable_status() == ASK_LATER:
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


################################################################
class android_code_gen(Task):
    """
    Generates the R.java files from the Android resources
    """
    color = 'PINK'
    run_str = '${AAPT} package -f -m ${APPT_RESOURCE_ST:APPT_RESOURCES} -M ${ANDROID_MANIFEST} ${APPT_INLC_ST:APPT_INCLUDES} -J ${OUTDIR} --generate-dependencies ${APP_PACKAGE_FLAGS}'
    vars = [ 'AAPT', 'APPT_RESOURCES', 'APPT_INCLUDES', 'APP_PACKAGE_FLAGS' ]


    def runnable_status(self):

        if not self.inputs:
            self.inputs  = []
            for resource in self.generator.appt_resources:
                res = resource.ant_glob('**/*')
                self.inputs.extend(res)

        result = super(android_code_gen, self).runnable_status()

        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    return RUN_ME

        return result


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
################################################################
@taskgen_method
def create_debug_strip_task(self, source_file, dest_location):

    lib_name = os.path.basename(source_file.abspath())
    output_node = dest_location.make_node(lib_name)

    # For Android Studio we should just copy the libs because stripping is part of the build process.
    # But we have issues with long path names that makes the stripping process to fail in Android Studio.
    self.create_task('strip_debug_symbols', source_file, output_node)


################################################################
@feature('c', 'cxx')
@after_method('apply_link')
def add_android_natives_processing(self):
    if 'android' not in self.env['PLATFORM']:
        return

    if not getattr(self, 'link_task', None):
        return

    if self._type == 'stlib': # Do not copy static libs
        return

    output_node = self.bld.get_output_folders(self.bld.env['PLATFORM'], self.bld.env['CONFIGURATION'])[0]

    project_name = getattr(self, 'project_name', None)
    game_projects = self.bld.get_enabled_game_project_list()

    for src in self.link_task.outputs:
        src_lib = output_node.make_node(src.name)

        for game in game_projects:

            game_build_native_key = '%s_BUILDER_NATIVES' % game

            # If the game is a valid android project, a specific build native value will have been created during
            # the project configuration.  Only process games with valid android project settings
            if not game_build_native_key in self.env:
                continue
            game_build_native_node = self.bld.root.find_dir(self.env[game_build_native_key])

            if self.bld.is_module_for_game_project(self.name, game, project_name):
                self.create_debug_strip_task(src_lib, game_build_native_node)


################################################################
@feature('c', 'cxx', 'copy_3rd_party_binaries')
@after_method('apply_link')
def add_3rd_party_library_stripping(self):
    """
    Strip and copy 3rd party shared libraries so they are included into the APK.
    """
    if 'android' not in self.env['PLATFORM'] or self.bld.spec_monolithic_build():
        return

    third_party_artifacts = self.env['COPY_3RD_PARTY_ARTIFACTS']
    if third_party_artifacts:
        game_projects = self.bld.get_enabled_game_project_list()
        for source_node in third_party_artifacts:
            _, ext = os.path.splitext(source_node.abspath())
            # Only care about shared libraries
            if ext == '.so':
                for game in game_projects:
                    game_build_native_key = '%s_BUILDER_NATIVES' % game

                    # If the game is a valid android project, a specific build native value will have been created during
                    # the project configuration.  Only process games with valid android project settings
                    if not game_build_native_key in self.env:
                        continue

                    game_build_native_node = self.bld.root.find_dir(self.env[game_build_native_key])
                    self.create_debug_strip_task(source_node, game_build_native_node)


################################################################
################################################################
@conf
def AndroidAPK(ctx, *k, **kw):

    project_name = kw['project_name']

    if (ctx.cmd == 'configure') or ('android' not in ctx.env['PLATFORM']) or (project_name not in ctx.get_enabled_game_project_list()):
        return

    Logs.debug('android: ******************************** ')
    Logs.debug('android: Processing {}...'.format(project_name))

    root_input = ctx.path.get_src().make_node('src')
    root_output = ctx.path.get_bld()

    env = ctx.env

    platform = env['PLATFORM']
    configuration = env['CONFIGURATION']

    # The variant name is constructed in the same fashion as how Gradle generates all it's build
    # variants.  After all the Gradle configurations and product flavors are evaluated, the variants
    # are generated in the following lower camel case format {product_flavor}{configuration}.
    # Our configuration and Gradle's configuration is a one to one mapping of what each describe,
    # while our platform is effectively Gradle's product flavor.
    gradle_variant = '{}{}'.format(platform, configuration.title())

    # copy over the required 3rd party libs that need to be bundled into the apk
    if ctx.options.from_android_studio:
        local_native_libs_node = root_input.make_node([ gradle_variant, 'jniLibs', env['ANDROID_ARCH'] ])
    else:
        local_native_libs_node = root_output.make_node([ 'builder', 'lib', env['ANDROID_ARCH'] ])

    local_native_libs_node.mkdir()
    Logs.debug('android: APK builder path (native libs) -> {}'.format(local_native_libs_node.abspath()))
    env['{}_BUILDER_NATIVES'.format(project_name)] = local_native_libs_node.abspath()

    libs_to_copy = env['EXT_LIBS']
    for lib in libs_to_copy:
        lib_name = os.path.basename(lib)

        dest = local_native_libs_node.make_node(lib_name)
        dest.delete()

        shutil.copy2(lib, dest.abspath())
        dest.chmod(FILE_PERMISSIONS)

    # since we are having android studio building the apk we can kick out early
    if ctx.options.from_android_studio:
        return

    local_variant_dirs = [ 'main', platform, configuration, gradle_variant ]

    java_source_nodes = []
    resource_nodes = []
    manifest_file = None

    for source_dir in local_variant_dirs:

        java_node = root_input.find_node([ source_dir, 'java' ])
        if java_node:
            java_source_nodes.append(java_node)

        res_node = root_input.find_node([ source_dir, 'res' ])
        if res_node:
            resource_nodes.append(res_node)

        manifest_node = root_input.find_node([ source_dir, 'AndroidManifest.xml' ])
        if manifest_node:
            if manifest_file:
                ctx.fatal('[ERROR] Multiple AndroidManifest.xml files per-project is not supported at this time')
            manifest_file = manifest_node

    if not manifest_file:
        ctx.fatal('[ERROR] Unable to find the AndroidManifest.xml in project path {}.'.format(ctx.path.get_src().abspath()))

    Logs.debug('android: Found local Java source directories {}'.format(java_source_nodes))
    Logs.debug('android: Found local resouce directories {}'.format(resource_nodes))
    Logs.debug('android: Found local manifest file {}'.format(manifest_file.abspath()))

    # get the keystore passwords
    if ctx.get_android_build_environment() == 'Distribution':
        key_pass = ctx.options.distro_key_pass
        store_pass = ctx.options.distro_store_pass

        if not (key_pass and store_pass):
            ctx.fatal('[ERROR] Build environment is set to Distribution but --distro-key-pass or --distro-store-pass arguments were not specified or blank')

    else:
        key_pass = ctx.options.dev_key_pass
        store_pass = ctx.options.dev_store_pass

    env['KEYPASS'] = key_pass
    env['STOREPASS'] = store_pass

    # Private function to add android libraries to the build
    def _add_library(folder, libName, resources, package_names, source_path):
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

        source_path.append(src)

        manifest =  folder.find_node('AndroidManifest.xml')
        if not manifest:
            Logs.error("[ERROR] Could not find the AndroidManifest.xml folder for library {}. Please check that they are present at {}".format(libName, folder.abspath()))
            return False

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

    library_jars = []
    library_packages = []

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
                        ctx.fatal("[ERROR] Required java lib - {} - was not found".format(jar_path))

            if 'patches' in value:
                lib_path = os.path.join(ctx.Path(ctx.get_android_patched_libraries_relative_path()), libName)
            else:
                # Search the multiple library paths where the library can be
                lib_path = None
                for path in value['srcDir']:
                    path = string.Template(path).substitute(env)
                    if os.path.exists(path):
                        lib_path = path
                        break

            if not _add_library(ctx.root.make_node(lib_path), libName, resource_nodes, library_packages, java_source_nodes):
                ctx.fatal("[ERROR] Could not add the android library - {}".format(libName))

    code_gen_out = root_output.make_node('gen')
    java_out = root_output.make_node('classes')
    apk_layout_dir = root_output.make_node('builder')

    Logs.debug('android: ****')
    Logs.debug('android: All Java source directories {}'.format(java_source_nodes))
    Logs.debug('android: All resouce directories {}'.format(resource_nodes))

    java_include_paths = java_source_nodes + [ code_gen_out ]
    java_source_paths = java_source_nodes

    ################################
    # Push all the Android apk packaging into their own build groups with
    # lazy posting to ensure they are processed at the end of the build
    ctx.post_mode = POST_LAZY
    build_group_name = '{}_android_group'.format(project_name)
    ctx.add_group(build_group_name)

    ctx(
        name = '{}_APK'.format(project_name),
        features = [ 'android', 'javac', 'use', 'uselib' ],

        game_project = project_name,

        # java settings
        compat          = env['JAVA_VERSION'],  # java compatibility version number
        classpath       = library_jars,

        #sourcepath      = java_include_paths,   # import search paths
        srcdir          = java_include_paths,   # folder containing the sources to compile
        outdir          = java_out,             # folder where to output the classes (in the build directory)

        # android settings
        android_manifest = manifest_file,
        java_gen_outdir = code_gen_out,

        appt_resources = resource_nodes,
        appt_extra_packages = library_packages,
        appt_package_flags = [ '--auto-add-overlay' ],
    )

    # reset the build group/mode back to default
    ctx.post_mode = POST_AT_ONCE
    ctx.set_group('regular_group')


################################################################
@feature('android')
@before('apply_java')
def apply_additional_java(self):

    java_gen_outdir = getattr(self, 'java_gen_outdir', None)
    if java_gen_outdir:
        if not isinstance(java_gen_outdir, Node.Node):
            java_gen_outdir = self.path.get_bld().make_node(java_gen_outdir)
    else:
        java_gen_outdir = self.path.get_bld().make_node('gen')

    java_gen_outdir.mkdir()

    self.java_gen_outdir = java_gen_outdir
    self.aidl_tasks = []

    aidl_src_paths = collect_source_paths(self, 'aidl_src_path')
    for srcdir in aidl_src_paths:
        for aidl_file in srcdir.ant_glob('**/*.aidl'):
            rel_path = aidl_file.path_from(srcdir)

            java_file = java_gen_outdir.make_node('{}.java'.format(os.path.splitext(rel_path)[0]))

            aidl_task = self.create_task('aidl', aidl_file, java_file)
            self.aidl_tasks.append(aidl_task)

    java_src_paths = collect_source_paths(self, 'java_src_path')
    append_to_unique_list(self.srcdir, java_src_paths)
    #append_to_unique_list(self.sourcepath, java_src_paths)

    Logs.debug('android: -> Additional Java source paths found {}'.format(java_src_paths))


################################################################
@feature('android')
@before_method('process_source')
@after_method('apply_java')
def apply_android(self):

    Utils.def_attrs(
        self,

        appt_resources = [],
        appt_extra_packages = [],
        appt_package_flags = [],
    )

    game_project_name = getattr(self, 'game_project', None)
    if not game_project_name:
        self.fatal('[ERROR] No "game_project" specified in Android package task.')

    self.code_gen_task = code_gen_task = self.create_task('android_code_gen')

    java_gen_outdir = self.java_gen_outdir
    code_gen_task.env['OUTDIR'] = java_gen_outdir.abspath()

    android_manifest = getattr(self, 'android_manifest', None)
    if not android_manifest:
        self.fatal('[ERROR] No "android_manifest" specified in Android package task.')

    if not isinstance(android_manifest, Node.Node):
        android_manifest = self.path.get_src().make_node(android_manifest)

    code_gen_task.env['ANDROID_MANIFEST'] = android_manifest.abspath()

    appt_resources = []
    for resource in self.appt_resources:
        if isinstance(resource, Node.Node):
            appt_resources.append(resource.abspath())
        else:
            appt_resources.append(resource)

    code_gen_task.env.append_value('APPT_RESOURCES', appt_resources)

    appt_package_flags = self.appt_package_flags

    extra_packages = self.appt_extra_packages
    if extra_packages:
        appt_package_flags.extend([ '--extra-packages', ':'.join(extra_packages) ])

    code_gen_task.env.append_value('APP_PACKAGE_FLAGS', appt_package_flags)

    game_package = self.bld.get_android_package_name(game_project_name)
    included_packages = extra_packages + [ game_package ]

    output_nodes = []
    for package in included_packages:
        sub_dirs = package.split('.')
        dir_path = os.path.join(*sub_dirs)

        r_java_path = os.path.join(dir_path, 'R.java')
        r_java_node = java_gen_outdir.make_node(r_java_path)

        output_nodes.append(r_java_node)

    code_gen_task.set_outputs(output_nodes)

    aidl_tasks = getattr(self, 'aidl_tasks', [])
    for aidl_task in aidl_tasks:
        code_gen_task.set_run_after(aidl_task)

    if hasattr(self, 'javac_task'):
        self.javac_task.set_run_after(self.code_gen_task)


################################################################
@feature('android')
@after_method('process_source')
def build_android_apk(self):
    """
    The heavy lifter of the APK packaging process.  Since this process is very order dependent and
    is required to happen at the end of the build (native and java), we push each of the required
    task into it's own build group with lazy posting to ensure that requirement.
    """
    project_name = self.game_project
    root_input = self.path.get_src()
    root_output = self.path.get_bld()

    platform = self.bld.env['PLATFORM']
    config = self.bld.env['CONFIGURATION']

    ################################################################
    # recreate the legacy variables to not break the APK building
    env = self.bld.env

    res_out = root_output.make_node('res')
    res_out.mkdir()

    resources = self.appt_resources[:]
    resources.append(res_out)

    resources_rule = ''
    for res in resources:
        resources_rule += '-S {} '.format(res.abspath().replace('\\', '/'))

    env['%s_J_OUT' % project_name] = self.outdir.abspath()
    env['%s_MANIFEST' % project_name] = self.android_manifest.abspath()

    builder_dir = root_output.make_node('builder')
    builder_dir.mkdir()
    env['%s_BUILDER_DIR' % project_name] = builder_dir.abspath()


    ################################################################
    def pull_shaders(shader_type, target_folder, last_task):
        """
        Helper function to pull the shaders from the device.
        """
        if adb_call('start-server') is None:
            Logs.warn("[WARN] Android pak shaders: Failed to start adb server. Skipping retrieving the shaders from device.")
            return None

        devices = get_list_of_android_devices()
        new_task = None
        if len(devices) == 0:
            Logs.warn("[WARN] Android pak shaders: there are no connected devices to pull the shaders. Skipping retrieving the shaders from device.")
            return None

        device_install_dir = "/sdcard"
        if hasattr(Options.options, 'deploy_android_root_dir'):
            device_install_dir = getattr(Options.options, 'deploy_android_root_dir')

        # We search for two paths because there's a bug that sometimes put the shaders in a different place
        base_path = "%s/%s/user" % (device_install_dir, project_name)
        source_paths = [base_path + '/cache/shaders/cache/' +  shader_type, base_path + '/shaders/cache/' + shader_type]

        for android_device in devices:
            for path in source_paths:
                ls_output = adb_call('shell', 'ls', path, device=android_device)
                if ls_output is None or 'Permission denied' in ls_output or 'No such file' in ls_output:
                    device_folder = None
                    continue
                else:
                    device_folder = path
                    break

            if not device_folder:
                continue

            if os.path.exists(target_folder.abspath()):
                try:
                    shutil.rmtree(target_folder.abspath(), ignore_errors = True)
                except:
                    Logs.warn("[WARN] Failed to delete %s folder to copy shaders" % target_folder )

            self.bld.env['%s_%s_SHADER_SOURCE_PATH' % (project_name, shader_type)] = device_folder
            self.bld.env['%s_%s_SHADER_TARGET_PATH' % (project_name, shader_type)] = '"' + target_folder.abspath() + '"'
            self.bld.env['%s_%s_SHADER_DEVICE' % (project_name, shader_type)] = android_device
            new_task = self.bld(
                name = 'pull_shaders',
                rule = ('adb -s ${%s_%s_SHADER_DEVICE} pull ${%s_%s_SHADER_SOURCE_PATH} ${%s_%s_SHADER_TARGET_PATH}') % ((project_name, shader_type) * 3),
                color = 'PINK',
                after = last_task,
                target = target_folder,
                always = True,
                shell = True)
            break

        if not new_task:
            Logs.warn("[WARN] Android pak shaders: could not pull shaders of type %s from any connected devices." % shader_type)

        return new_task

    ################################################################
    def add_asset_folder_to_apk(source_path, dest_path):
        """
        Helper function to create the correct junction and folder structure for adding assets to the APK.
        """
        assets_for_apk = root_input.make_node('assets_for_apk')
        assets_for_apk.mkdir()

        temp_folder = assets_for_apk.make_node(str(hashlib.sha1(source_path.abspath()).hexdigest()))
        temp_folder_path = temp_folder.abspath()
        intermediate_folder = temp_folder.make_node(os.path.dirname(dest_path))
        target = intermediate_folder.make_node(os.path.basename(dest_path)).abspath()

        # get rid of the target first - this ensures that on windows, we don't call rmtree on something that contains junctions
        if Utils.unversioned_sys_platform() == "win32":
            remove_junction = os.rmdir
        else:
            remove_junction = os.unlink

        if (os.path.exists(target) and is_directory_junction(target)):
            remove_junction(target)

        # now get rid of the intermediate folder if necessary
        if os.path.exists(temp_folder_path):
            Logs.debug('android: temp folder already exists %s, will try to remove it' % (temp_folder_path))
            try:
                if is_directory_junction(temp_folder_path):
                    remove_junction(target)
                else:
                    shutil.rmtree(temp_folder_path, ignore_errors = True)
            except Exception as inst:
                pass

        # Generate an intermediate folder where we will add the symlink to the real folder

        # Don't create the folder if it's going to be the symlink
        if target != intermediate_folder.abspath():
            Logs.debug('android: Making intermediate folder ' + intermediate_folder.abspath())
            intermediate_folder.mkdir()

        try:
            Logs.debug('android: symlinking "%s" --> "%s"' % (source_path.abspath(), target.lower()))
            junction_directory(source_path.abspath(), target.lower())
        except:
            self.bld.fatal("[ERROR] Could not create junction for asset folder %s" % source_path.abspath())
            return ''

        return intermediate_folder.abspath()


    # set the new build group for apk building
    self.bld.post_mode = POST_LAZY
    self.bld.add_group('%s_apk_builder_group' % project_name)

    apk_out = root_output.make_node('apk')
    final_apk_out = self.bld.get_output_folders(self.bld.env['PLATFORM'], self.bld.env['CONFIGURATION'])[0]

    pacakge_name = self.bld.get_android_package_name(project_name)
    app_version_number = self.bld.get_android_version_number(project_name)

    ################################
    # create the dalvik executable
    self.bld(
        name = 'dex',
        rule = '${DX} --dex --output ${TGT} ${%s_J_OUT}' % project_name,
        target = root_output.make_node('builder/classes.dex'),
        color = 'PINK',
        always = True,
        shell = False)

    ################################
    # handle the standard application assets
    self.bld(
        name = 'crunch',
        rule = '${AAPT} crunch ' + resources_rule + ' -C ${TGT}',
        target = res_out,
        after = 'dex',
        color = 'PINK',
        always = True,
        shell = False)

    pack_resources = self.bld(
        name = 'package_resources',
        rule = '${AAPT} package --no-crunch -f ${ANDROID_DEBUG_MODE} -0 apk ' + resources_rule +' -M ${%s_MANIFEST} ${APPT_INLC_ST:APPT_INCLUDES} -F ${TGT} --generate-dependencies --auto-add-overlay' % ((project_name,) * 1),
        target = root_output.make_node('bin/%s.ap_' % project_name),
        after = 'crunch',
        color = 'PINK',
        always = True,
        shell = False)

    prev_task = pack_resources.name

    ##################################################
    # Determine if we need to pack assets into the APK.
    pack_assets_in_apk = self.bld.get_android_place_assets_in_apk_setting(project_name)
    asset_folders = []
    if pack_assets_in_apk:
        assets = self.bld.get_bootstrap_assets("android")
        asset_dir = "cache/{}/{}".format(project_name, assets).lower()
        asset_node = self.bld.path.find_dir(asset_dir)

        if asset_node == None:
            self.bld.fatal("[ERROR] Could not find Asset Processor cache at [%s] - you may need to run the Asset Processor to create the assets" % asset_dir )
            return

        use_paks = config == 'release' or (self.bld.is_option_true('deploy_android') and self.bld.is_option_true('deploy_android_paks'))
        if use_paks:
            ##################################################
            # Create pak files and Obb file
            rc_path = get_resource_compiler_path(self.bld)
            if not rc_path:
                self.bld.fatal("[ERROR] Resource Compiler was not found. Please build it from the command line.")

            self.bld.env['RC'] = rc_path
            use_obb = 'true' in [self.bld.get_android_use_main_obb(project_name).lower(), self.bld.get_android_use_patch_obb(project_name).lower()]
            if use_obb:
                # Even though the executable of the RC is in Bin64vs120\rc or Bin64vs140\rc, the job files still are in Bin64\rc.
                rc_job_file = self.bld.path.find_resource('Bin64/rc/%s' % self.bld.get_android_rc_obb_job(project_name))
                self.bld.env['%s_RC_OBB' % project_name] = '/obb_pak="%s"' % final_apk_out.make_node('main.{}.{}.obb'.format(app_version_number, pacakge_name)).abspath()
                self.bld.env['%s_RC_TEMP' % project_name] = '/tmp="%s"' % self.bld.path.make_node('RcTemp/%s' % project_name).abspath()
                rc_target = self.bld.path.make_node('AndroidArmv7LayoutObb/%s' % project_name)
            else:
                rc_job_file = self.bld.path.find_resource('Bin64/rc/%s' % self.bld.get_android_rc_pak_job(project_name))
                rc_target = self.bld.path.make_node('AndroidArmv7LayoutPak/%s' % project_name)

            if not rc_job_file:
                self.bld.fatal("[ERROR] Could not find resource file for creating the pak files")

            self.bld.env['%s_RC_JOB' % project_name] = '/job="%s"' % rc_job_file.abspath()
            self.bld.env['%s_RC_TARGET' % project_name] = '/trg="%s"' % rc_target.abspath()
            self.bld.env['%s_RC_SRC' % project_name] = '/src="%s"' % asset_node.abspath()

            rc_task = self.bld( name = 'build_paks',
                    rule = ('${RC} ${%s_RC_JOB} ${%s_RC_SRC} ${%s_RC_TARGET} ${%s_RC_TEMP} ${%s_RC_OBB} /p=' +  assets + ' /threads=4 /game=' + project_name.lower()) % ((project_name,) * 5),
                    color = 'PINK',
                    after = prev_task,
                    target = rc_target,
                    always = True,
                    shell = True)
            prev_task = rc_task.name
            rc_target.mkdir()
            # The assets now are the pak files created by the RC.
            asset_node = rc_target

        ################################
        # Add assets folder to assets
        asset_folders.append(add_asset_folder_to_apk(asset_node, "/"))

        shader_types = ['gles3_0', 'gles3_1']

        ################################
        # Pull shaders from device
        shaders_source_paths = dict()
        # Check if we already have a shader pak in the asset's cache.
        if asset_node.find_resource('%s/shaders.pak' % project_name) or asset_node.find_resource('%s/shaderscache.pak' % project_name):
            Logs.warn("[WARN] Skipping creating the shaders pak because the file already exist in the cache folder.")
        else:
            # Check if we already have the shader's source files or we need to pull them from the device.
            for shader_flavor in shader_types:
                shader_user_node = asset_node.find_dir("user/cache/shaders/cache/{}".format(shader_flavor).lower())
                if shader_user_node:
                    shaders_source_paths[shader_flavor] = shader_user_node
                    Logs.debug("android: skipping pulling shaders of type %s from device. Using 'user' folder instead." % shader_flavor)
                else:
                    pull_shaders_folder = self.bld.path.make_node("build/temp/{}/{}/{}".format(assets, project_name, shader_flavor).lower())
                    pull_shaders_task = pull_shaders(shader_flavor, pull_shaders_folder, prev_task)
                    if pull_shaders_task:
                        # We found a device with a valid shader folder.
                        prev_task = pull_shaders_task.name
                        shaders_source_paths[shader_flavor] = pull_shaders_folder

        ################################
        # Create the shader pak file
        if shaders_source_paths:
            # Need to execute the python script as a task
            if Utils.unversioned_sys_platform() == "win32":
                python_cmd = '"' + self.bld.path.find_resource('Tools/Python/python.cmd').abspath() + '"'

            else:
                python_cmd = 'python'

            pak_script = self.bld.path.find_resource('Tools/PakShaders/pak_shaders.py')
            shaders_pak_dir = self.bld.path.make_node("build/{}/{}".format(assets, project_name).lower())
            shaders_pak_dir.mkdir()

            self.bld.env['PYTHON_CMD'] = python_cmd
            self.bld.env['PAK_SCRIPT'] = '"' + pak_script.abspath() + '"'
            self.bld.env['%s_SHADER_TYPES' % project_name] = [key + ',"' + path.abspath() + '"' for key, path in shaders_source_paths.iteritems()]
            self.bld.env['%s_PAK_OUTPUT' % project_name] = '"' + shaders_pak_dir.abspath() + '"'

            shaders_pak = self.bld(
                name = 'build_shaders_pak',
                rule = ('${PYTHON_CMD} ${PAK_SCRIPT} ${%s_PAK_OUTPUT} -s ${%s_SHADER_TYPES}') % ((project_name,) * 2),
                color = 'PINK',
                target = shaders_pak_dir,
                cwd = self.bld.path.abspath(),
                after = prev_task,
                always = True,
                shell = True)

            prev_task = shaders_pak.name

            ################################
            # Add shaders paks to assets
            # Insert at the beginning so it has precedence in case of file name collision.
            asset_folders.insert(0, add_asset_folder_to_apk(shaders_pak_dir, "/%s" % project_name))

        else:
            Logs.warn('[WARN] Skipping packing shaders.')

    ################################
    # generate the APKs

    # Generating all the APK has to be in the right order.  This is important for Android store APK uploads,
    # if the alignment happens before the signing, then the signing will blow over the alignment and will
    # require a realignment before store upload.
    # 1. Generate the unsigned, unaligned APK
    # 2. Sign the APK
    # 3. Align the APK

    asset_rule = ''
    for index, asset_path in enumerate(asset_folders):
        envVarName = '%s_ASSETS_%d' % (project_name, index)
        self.bld.env[envVarName] = asset_path
        asset_rule += '-A ${%s} ' % envVarName

    rule_str = ('${AAPT} package -f ' + resources_rule +' -M ${%s_MANIFEST} ${ANDROID_DEBUG_MODE} ${APPT_INLC_ST:APPT_INCLUDES} -F ${TGT} ' + asset_rule + ' --auto-add-overlay ${%s_BUILDER_DIR}') % ((project_name,) * 2)

    # 1. Generate the unsigned, unaligned APK
    raw_apk = self.bld(
        name = 'building_apk',
        rule = rule_str,
        target = apk_out.make_node('%s_unaligned_unsigned.apk' % project_name),
        after = prev_task,
        color = 'PINK',
        always = True,
        shell = False)

    # 2. Sign the APK
    signed_apk = self.bld(
        name = 'signing_apk',
        rule = '${JARSIGNER} -keystore ${KEYSTORE} -storepass ${STOREPASS} -keypass ${KEYPASS} -signedjar ${TGT} ${SRC} ${KEYSTORE_ALIAS}',
        source = raw_apk.target,
        target = apk_out.make_node('%s_unaligned.apk' % project_name),
        after = 'building_apk',
        color = 'PINK',
        always = True,
        shell = False)

    # 3. Align the APK
    self.bld(
        name = 'aligning_apk',
        rule = '${ZIPALIGN} -f 4 ${SRC} ${TGT}',
        source = signed_apk.target,
        target = final_apk_out.make_node('%s.apk' % project_name),
        after = 'signing_apk',
        color = 'PINK',
        always = True,
        shell = False)

    # reset the build group/mode back to default
    self.bld.post_mode = POST_AT_ONCE
    self.bld.set_group('regular_group')


###############################################################################
def adb_call(cmd, *args, **keywords):
    ''' Issue a adb command. Args are joined into a single string with spaces
    in between and keyword arguments supported is device=serial # of device
    reported by adb.
    Examples:
    adb_call('shell', 'ls', '-ld') results in "adb shell ls -ld" being executed
    adb_call('shell', 'ls', device='123456') results in "adb -s 123456 shell ls" being executed
    '''
    device_option = ''
    if 'device' in keywords:
        device_option = "-s " + keywords['device']

    cmdline = 'adb {} {} {}'.format(device_option, cmd, ' '.join(list(args)))
    Logs.debug('adb_call: ADB Cmdline: ' + cmdline)
    try:
        return check_output(cmdline, stderr = subprocess.STDOUT, shell = True)
    except Exception as inst:
        Logs.debug('adb_call: exception was thrown: ' + str(inst))
        return None  #Return None so the caller can handle the failure gracefully


class adb_copy_output(Task):
    ''' Class to handle copying of a single file in the layout to the android
    device.
    '''

    def __init__(self, *k, **kw):
        Task.__init__(self, *k, **kw)
        self.device = ''
        self.target = ''

    def set_device(self, device):
        '''Sets the android device (serial number from adb devices command) to
        copy the file to'''
        self.device = device

    def set_target(self, target):
        '''Sets the target file directory (absolute path) and file name on the device'''
        self.target = target

    def run(self):
        # Embed quotes in src/target so that we can correctly handle spaces
        src = '"' + self.inputs[0].abspath() + '"'
        tgt = '"' + self.target + '"'

        Logs.debug("adb_copy_output: performing copy - %s to %s on device %s" % (src, tgt, self.device))
        adb_call('push', src, tgt, device=self.device)

        return 0

    def runnable_status(self):
        if Task.runnable_status(self) == ASK_LATER:
            return ASK_LATER

        return RUN_ME

@taskgen_method
def adb_copy_task(self, android_device, src_node, output_target):
    '''Create a adb_copy_output task to copy the src_node to the ouput_target
    on the specified device. output_target is the absolute path and file name
    of the target file.
    '''
    copy_task = self.create_task('adb_copy_output', src_node)
    copy_task.set_device(android_device)
    copy_task.set_target(output_target)


###############################################################################
# create a deployment context for each build variant
for configuration in ['debug', 'profile', 'release']:
    for compiler in ['clang', 'gcc']:
        class DeployAndroidContext(Build.BuildContext):
            fun = 'deploy'
            variant = 'android_armv7_' + compiler + '_' + configuration
            after = ['build_android_armv7_' + compiler + '_' + configuration]
            cmd = 'deploy_android_armv7_' + compiler + '_' + configuration

            def get_bootstrap_files(self):
                config_path = self.get_bootstrap_game().lower() + '/config/'
                config_path += 'game.xml'
                return ['bootstrap.cfg', config_path]

            def use_vfs(self):
                try:
                    return self.cached_use_vfs
                except:
                    self.cached_use_vfs = self.get_bootstrap_vfs() == '1'

                return self.use_vfs()

            def use_paks(self):
                try:
                    return self.cached_use_paks
                except:
                    (platform, configuration) = self.get_platform_and_configuration()
                    self.cached_use_paks = configuration == 'release' or self.is_option_true('deploy_android_paks')

                return self.use_paks()

            def get_layout_node(self):
                try:
                    return self.android_armv7_layout_node
                except:
                    if self.use_vfs():
                        self.android_armv7_layout_node = self.path.make_node('AndroidArmv7LayoutVFS')
                    elif self.use_paks():
                        self.android_armv7_layout_node = self.path.make_node('AndroidArmv7LayoutPak')
                    else:
                        # For android we don't have to copy the assets to the layout node, so just use the cache folder
                        game = self.get_bootstrap_game()
                        assets = self.get_bootstrap_assets("android")
                        asset_cache_dir = "cache/{}/{}".format(game, assets).lower()
                        self.android_armv7_layout_node = self.path.find_dir(asset_cache_dir)
                        if (self.android_armv7_layout_node == None):
                            Logs.warn("[WARN] android deploy: There is no asset cache to read from at " + asset_cache_dir)
                            Logs.warn("[WARN] android deploy: You probably need to run Bin64/AssetProcessorBatch.exe")

                return self.get_layout_node()

        class DeployAndroid(DeployAndroidContext):
            after = ['build_android_armv7_' + compiler + '_' + configuration]
            cmd = 'deploy_android_armv7_' + compiler + '_' + configuration
            features = ['deploy_android_prepare']

        class DeployAndroidDevices(DeployAndroidContext):
            after = ['deploy_android_armv7_' + compiler + '_' + configuration]
            cmd = 'deploy_android_devices_' + compiler + '_' + configuration
            features = ['deploy_android_devices']

def get_list_of_android_devices():
    ''' Gets the connected android devices using the adb command devices and
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

@taskgen_method
@feature('deploy_android_prepare')
def prepare_to_deploy_android(ctx):
    '''Deploy an android build to connected android devices. This will create
    the AndroidLayout* folder first with all the assets in it, which is then
    copied to the device location specified in the deploy_android_root_dir
    option.
    '''

    color = 'CYAN'
    bld = ctx.bld
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    if platform not in ('android_armv7_gcc', 'android_armv7_clang'):
        return

    if not bld.is_option_true('deploy_android') or bld.options.from_android_studio:
        Logs.pprint(color, 'Skipping Android Deployment...')
        return

    if bld.use_vfs() and configuration == 'release':
        bld.fatal('Cannot use VFS in a release build, please set remote_filesystem=0 in bootstrap.cfg')

    game = bld.get_bootstrap_game()

    if bld.use_paks() and not bld.get_android_place_assets_in_apk_setting(game):
        assets = bld.get_bootstrap_assets("android")
        asset_cache_dir = "cache/{}/{}".format(game, assets).lower()
        asset_cache_node = bld.path.find_dir(asset_cache_dir)

        if (asset_cache_node != None):
            layout_node = bld.get_layout_node()

            call('{} /job={} /p=es3 /src={} /trg={} /threads=8 /game={}'.format(os.path.normpath(get_resource_compiler_path(bld)),
                                                                                os.path.join('Bin64', 'rc', bld.get_android_rc_pak_job(game)),
                                                                                asset_cache_dir,
                                                                                layout_node.relpath().lower(),
                                                                                game), shell=True)
        else:
            Logs.warn("[WARN] android deploy preparation: Layout generation is enabled, but there is no asset cache to read from at " + asset_cache_dir)
            Logs.warn("[WARN] android deploy preparation: You probably need to run Bin64/AssetProcessor.exe (GUI) or Bin64/AssetProcessorBatch.exe")


    compiler = platform.split('_')[2]
    Options.commands.append('deploy_android_devices_' + compiler + '_' + configuration)


@taskgen_method
@feature('deploy_android_devices')
def deploy_to_devices(ctx):
    '''Installs the project APK and copies the layout directory to all the
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

    def deploy_error(message):
        if ctx.bld.options.from_editor_deploy:
            ctx.bld.fatal('[ERROR] android deploy: %s' % message)
        else:
            Logs.warn('[WARN] android deploy: %s' % message)

    # make sure the adb server is running first before we execute any other
    # commands
    if adb_call('start-server') is None:
        deploy_error("failed to start adb server. Skipping deploy step.")
        return

    devices = get_list_of_android_devices()
    if len(devices) == 0:
        deploy_error("there are no connected devices to deploy a build to. Skipping deploy step.")
        return

    color = 'CYAN'
    bld = ctx.bld
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    game = bld.get_bootstrap_game()

    # get location of APK either from command line option or default build location
    if Options.options.apk_path == '':
        output_folder = bld.get_output_folders(platform, configuration)[0]
        apk_name = output_folder.abspath() + "/" + game + ".apk"
    else:
        apk_name = Options.options.apk_path

    device_install_dir = "/storage/sdcard0/"
    if hasattr(Options.options, 'deploy_android_root_dir'):
        device_install_dir = getattr(Options.options, 'deploy_android_root_dir')

    output_target = "%s/%s/" % (device_install_dir, game)

    # This is the name of a file that we will use as a marker/timestamp. We
    # will get the timestamp of the file off the device and compare that with
    # asset files on the host machine to determine if the host machine asset
    # file is newer than what the device has, and if so copy it to the device.
    timestamp_file_name = "engineroot.txt"
    device_timestamp_file = output_target + timestamp_file_name

    do_clean = bld.is_option_true('deploy_android_clean_device')
    deploy_executable = bld.is_option_true("deploy_android_executable")
    pack_assets_in_apk = bld.get_android_place_assets_in_apk_setting(game)
    deploy_assets = bld.is_option_true("deploy_android_assets") and not bld.use_vfs() and not pack_assets_in_apk
    layout_node = bld.get_layout_node()

    if bld.is_option_true("deploy_android_assets") and pack_assets_in_apk:
        Logs.warn('[WARN] android deploy: Skipping deploying assets because they were placed inside the APK')

    Logs.debug('android: deploy options: do_clean %s, deploy_exec %s, deploy_assets %s' % (do_clean, deploy_executable, deploy_assets))

    # Some checkings before we start the deploy process.
    if deploy_assets and layout_node is None:
        deploy_error('Could not find the Assets folder. You may need to run the Asset Processor to create the assets. Skipping deploy step.')
        return

    if deploy_executable and not os.path.exists(apk_name):
        deploy_error('Could not find executable (APK) for deployment (%s). Skipping deploy step.' % apk_name)
        return

    deploy_count = 0

    for android_device in devices:
        Logs.pprint(color, 'Starting to deploy to android device ' + android_device)

        if do_clean:
            Logs.pprint(color, 'Cleaning target before deployment...')
            adb_call('shell', 'rm -rf ', output_target, device=android_device)
            Logs.pprint(color, 'Target Cleaned...')

            package_name = bld.get_android_package_name(game)
            if len(package_name) != 0 and deploy_executable:
                Logs.pprint(color, 'Uninstalling package ' + package_name)
                adb_call('uninstall', package_name, device=android_device)

        if deploy_executable:
            install_options = getattr(Options.options, 'deploy_android_install_options')
            replace_package = ''
            if bld.is_option_true('deploy_android_replace_apk'):
                replace_package = '-r'

            Logs.pprint(color, 'Installing ' + apk_name)
            install_result = adb_call('install', install_options, replace_package, '"' + apk_name + '"', device=android_device)
            if not install_result or 'success' not in install_result.lower():
                Logs.warn('[WARN] android deploy: failed to install APK on device %s.' % android_device)
                if install_result:
                    # The error msg is the last non empty line of the output.
                    error_msg = next(error for error in reversed(install_result.splitlines()) if error)
                    Logs.warn('[WARN] android deploy: %s' % error_msg)
                continue

        if deploy_assets and layout_node is not None:

            ls_output = adb_call('shell', 'ls', device_install_dir, device=android_device)
            if ls_output is None or 'Permission denied' in ls_output or 'No such file' in ls_output:
                Logs.warn('[WARN] android deploy: unable to access the destination %s on the target device %s - skipping deploying assets. Make sure deploy_android_root_dir option is valid for the device' % (output_target, android_device))
                continue

            ls_output = adb_call('shell', 'ls -l ', device_timestamp_file, device=android_device)
            target_time = 0
            if ls_output is not None and "No such file" not in ls_output:
                tgt_ls_fields = ls_output.split()
                target_time = time.mktime(time.strptime(tgt_ls_fields[4] + " " + tgt_ls_fields[5], "%Y-%m-%d %H:%M"))
                Logs.debug('android: %s time is %s' % (device_timestamp_file, target_time))

            if do_clean or target_time == 0:
                Logs.pprint(color, 'Copying all assets to the device %s. This may take some time...' % android_device)

                # There is a chance that if someone ran VFS before this deploy
                # on an empty directory the output_target directory will have
                # already existed when we do the push command. In this case if
                # we execute adb push command it will create an ES3 directory
                # and put everything there, causing the deploy to be
                # 'successful' but the game will crash as it won't be able to
                # find the assets. Since we detected a "clean" build, wipe out
                # the output_target folder if it exists first then do the push
                # and everything will be just fine.
                ls_output = adb_call('shell', 'ls', output_target)
                if ls_output is not None and "No such file" not in ls_output:
                    adb_call('shell rm -rf', output_target)

                if adb_call('push', '"' + layout_node.abspath() + '"', output_target) is None:
                    # Clean up any files we may have pushed to make the next
                    # run success rate better
                    adb_call('shell rm -rf', output_target)
                    deploy_error('adb command to push all the files to the device failed.')
                    continue

            else:
                layout_files = layout_node.ant_glob('**/*')
                Logs.pprint(color, 'Scanning %d files to determine which ones need to be copied...' % len(layout_files))
                for src_file in layout_files:
                    # Faster to check if we should copy now rather than in the copy_task
                    if should_copy_file(src_file, target_time):
                        final_target_dir = output_target + string.replace(src_file.path_from(layout_node), '\\', '/')
                        ctx.adb_copy_task(android_device, src_file, final_target_dir)

                # Push the timestamp_file_name last so that it has a timestamp
                # that we can use on the next deploy to know which files to
                # upload to the device
                host_timestamp_file = ctx.bld.path.find_node(timestamp_file_name)
                adb_call('push', '"' + host_timestamp_file.abspath() + '"', device_timestamp_file, device=android_device)

        if not pack_assets_in_apk:
            Logs.pprint(color, 'Copying required files for booting ...' )
            for bs_file in bld.get_bootstrap_files():
                bs_src = ctx.bld.path.find_node(bs_file)
                if bs_src != None:
                    target_file_with_dir = output_target + bs_src.relpath().replace('\\', '/')
                    ls_output = adb_call('ls', target_file_with_dir, device=android_device)
                    if 'Permission denied' in ls_output or 'No such file' in ls_output:
                        adb_call('mkdir', target_file_with_dir, device=android_device)
                    adb_call('push', '"' + bs_src.abspath() + '"', target_file_with_dir, device=android_device)

        deploy_count = deploy_count + 1

    if deploy_count == 0:
        deploy_error("could not deploy the build to any connected devices.")

