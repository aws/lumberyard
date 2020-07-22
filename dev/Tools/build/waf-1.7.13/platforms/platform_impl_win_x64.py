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
import os
import subprocess
import sys
import re

# waflib imports
from waflib import Configure, Errors, Logs, Utils
from waflib.Configure import conf, Logs

# lmbrwaflib imports
from lmbrwaflib.utils import parse_json_file, write_json_file

# misc imports
from waf_branch_spec import LMBR_WAF_VERSION_TAG, BINTEMP_CACHE_TOOLS


VC_COMPILER_NAME = 'CL'
VC_LINKER_NAME = 'LINK'
VC_RC_NAME = 'RC'
VC_MT_NAME = 'mt'
VC_AROOL_NAME = 'LIB'


if not Utils.winreg:
    raise Errors.WafError("This platform target is not supported on this host platform")


@Configure.conf
def run_unittest_launcher_for_win_x64(ctx, game_project_name):
    """
    Helper context function to execute the unit test launcher for a specific game project

    :param ctx:                 Context
    :param game_project_name:   The current project name (extracted from bootstrap.cfg)

    """
    
    output_folder = ctx.get_output_folders(ctx.platform, ctx.config)[0]
    current_project_launcher = ctx.env['cprogram_PATTERN'] % '{}Launcher'.format(game_project_name)
    current_project_unittest_launcher_fullpath = os.path.join(output_folder.abspath(), current_project_launcher)
    if not os.path.isfile(current_project_unittest_launcher_fullpath):
        raise Errors.WafError("Unable to launch unit tests for project '{}'. Cannot find launcher file '{}'. Make sure the project has been built successfully.".format(game_project_name, current_project_unittest_launcher_fullpath))
    Logs.info('[WAF] Running unit tests for {}'.format(game_project_name))
    
    try:
        call_args = [current_project_unittest_launcher_fullpath]

        if not ctx.is_engine_local():
            call_args.extend(['--app-root', ctx.get_launch_node().abspath()])

        # Grab any optional arguments
        auto_launch_unit_test_arguments = ctx.get_settings_value('auto_launch_unit_test_arguments')
        if auto_launch_unit_test_arguments:
            call_args.extend(auto_launch_unit_test_arguments.split(' '))

        result_code = subprocess.call(call_args)
    except Exception as e:
        raise Errors.WafError("Error executing unit tests for '{}': {}".format(game_project_name, e))
    if result_code != 0:
        raise Errors.WafError("Unit tests for '{}' failed. Return code {}".format(game_project_name, result_code))
    else:
        Logs.info('[WAF] Running unit tests for {}'.format(game_project_name))


class VisualStudioPlatformDetails(object):
    """
    Simple type to manage Visual Studio Platform Details collected during the detection process
    """

    def __init__(self, in_platform, in_arch, in_msvs_ver, in_compiler, in_vs_version):
        self.platform = in_platform
        self.arch = in_arch
        self.msvs_ver = in_msvs_ver
        self.compiler = in_compiler
        self.vs_version = in_vs_version


VS_PATTERN_MATCHER = re.compile(r'([\w]*)(vs)(\d{4})')


@conf
def get_visual_studio_platform_map(ctx):
    """
    Get the (cached) map of waf platform name to a Visual Studio Platform Details
    :param ctx:     Context
    :return:        Map of waf platform name -> Visual Studio Platform Details
    """
    try:
        return ctx.visual_studio_platform_map
    except AttributeError:

        visual_studio_platform_map = {}

        all_platform_names = ctx.get_all_platform_names()
        for platform_name in all_platform_names:

            vs_pattern = VS_PATTERN_MATCHER.match(platform_name)
            if not vs_pattern:
                continue
            if vs_pattern.lastindex < 3:
                continue
            if str(vs_pattern.group(2)).lower()!='vs':
                continue
            vs_version = 'VS{}'.format(str(vs_pattern.group(3)))

            platform_details = ctx.get_platform_settings(platform_name)
            if not platform_details.attributes.get('is_windows', False):
                continue

            msvs_attributes =  platform_details.attributes.get('msvs', None)
            if not msvs_attributes:
                continue

            visual_studio_platform_map[platform_name] = VisualStudioPlatformDetails(in_platform=platform_name,
                                                                                    in_arch=msvs_attributes['toolset_name'],
                                                                                    in_msvs_ver=msvs_attributes['msvs_ver'],
                                                                                    in_compiler=platform_details.attributes['compiler'],
                                                                                    in_vs_version=vs_version)
        ctx.visual_studio_platform_map = visual_studio_platform_map
        return ctx.visual_studio_platform_map


def detect_vswhere_path():
    """
    Attempt to detect the location of vswhere, which is used to query the installed visual studio tools (version 2017+)
    :return: The validated path to vswhere
    """
    # Find VS Where
    path_program_files_x86 = os.environ['ProgramFiles(x86)']
    if not path_program_files_x86 or not os.path.isdir(path_program_files_x86):
        raise Errors.WafError("Unable to determine folder 'Program Files (x86)'")
    path_visual_studio_installer = os.path.normpath(os.path.join(path_program_files_x86, 'Microsoft Visual Studio\\Installer\\'))
    if not os.path.isdir(path_visual_studio_installer):
        raise Errors.WafError("Unable to locate Visual Studio Installer.")
    path_vswhere = os.path.normpath(os.path.join(path_visual_studio_installer, 'vswhere.exe'))
    if not os.path.isfile(path_vswhere):
        raise Errors.WafError("Unable to locate 'vswhere' in path '{}'.".format(path_visual_studio_installer))
    return path_vswhere


def execute_vs_query(command):
    """
    Simple execute command and return the result in a string
    :param command: The command to execute
    :return: The stdout of the result of the command
    """

    proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=None)
    proc.wait()
    result_string = proc.stdout.read().decode(sys.stdout.encoding or 'iso8859-1', 'ignore')
    if not result_string:
        return None

    return result_string.strip()


def execute_vs_query_into_map(command):
    """
    Execute a vs query command and store the result ( list of <key>: <value> ) into a map
    :param command: The vs query command to execute
    :return: The map of the result
    """

    result_string = execute_vs_query(command)
    if not result_string:
        return {}

    result_map = {}
    lines = [line.strip() for line in result_string.split('\n')]
    for line in lines:
        first_sep = line.find(':')
        if first_sep <= 0:
            continue
        key = line[:first_sep].strip()
        val = line[first_sep+1:].strip()

        result_map[key] = val

    return result_map


VSWHERE_PRODUCT_IDS_BY_PRIORITY = [
    'Microsoft.VisualStudio.Product.Enterprise',
    'Microsoft.VisualStudio.Product.Professional',
    'Microsoft.VisualStudio.Product.Community',
    'Microsoft.VisualStudio.Product.BuildTools'
]


@conf
def query_vswhere_versions(ctx, vswhere_args):
    """
    Use vswhere to collect the a visual studio version into a mapped result table (generated from vswhere)
    :param ctx:          Context
    :param vswhere_args: Additional arguments to pass into vswhere
    :return: Map of the query result if successful, None if VS product version is not installed
    """

    vswhere_path = detect_vswhere_path()

    # Turn vswhere args into a list of value
    if isinstance(vswhere_args, str):
        vswhere_args = vswhere_args.split()

    # Walk through the list of product priorities and attempt to detect the product
    detected_product_id = None
    detected_install_version = None

    for check_product_id in VSWHERE_PRODUCT_IDS_BY_PRIORITY:
        command = [vswhere_path, '-products', check_product_id, '-property', 'installationVersion', '-latest', '-nologo'] + vswhere_args
        detect_version_result = execute_vs_query(command)
        # Two vswhere query commands are needed to retrieve the version and product id
        # as the -property switch can only return one property at a time
        if not detect_version_result:
            continue
        detected_install_version = detect_version_result

        # retrieve the productId from the query result. This should never fail as the above check validated
        # that the visual studio product is installed for the specified version
        # vswhere_args can add additional -products to check, a second query is needed to retrieve the one that is installed
        command = [vswhere_path, '-products', check_product_id, '-property', 'productId', '-latest', '-nologo'] + vswhere_args
        detect_product_result = execute_vs_query(command)
        if detect_product_result:
            detected_product_id = detect_product_result
            break

    if not detected_product_id or not detected_install_version:
        return None

    # If we detected a viable installed visual studio, then read in the entire value based on the exact installation version
    # Added the vswhere_args before the using the found version and product id to query the vswhere properties
    # This allows the found -version and -products values to override the ones within the vswhere_args array, while
    # still allowing the other vswhere_args to be used
    command = [vswhere_path] + vswhere_args + ['-version', '[{}]'.format(detected_install_version), '-products', detected_product_id, '-nologo']
    results = execute_vs_query_into_map(command)

    return results


@conf
def query_vcvars_all(ctx, platform_name, vcvars_path, compiler_version, winkit_version, vcvarsall_args):
    """
    Use the detected vcvarsall.bat to determine the include paths, lib paths, and executable paths for a particular version of visual studio

    :param ctx:                 Context
    :param platform_name:       The version of visual studio this detection is being processed for
    :param vcvars_path:         The full path to the vcvarsall.bat file that comes with each visual studio installation
    :param compiler_version:    The version (architecture) of the compiler tools to determine
    :param winkit_version:      The windows kit version to include in the paths
    :param vcvarsall_args:      Additional arguments to supply vcvarsall.bat(Support options are -vcvars_ver=vc_version and
                                -vcvars_spectre_libs=spectre_mode
    :return: Tuple of ( executable paths, include paths, and lib paths )
    """

    vs_detect_bat_file = ctx.bldnode.make_node('waf-print-msvc-{}.bat'.format(platform_name))

    vs_detect_bat_file.write("""@echo off
set INCLUDE=
set LIB=
call "%s" %s %s %s
echo PATH=%%PATH%%
echo INCLUDE=%%INCLUDE%%
echo LIB=%%LIB%%;%%LIBPATH%%
""" % (vcvars_path, compiler_version, winkit_version, vcvarsall_args))
    sout = ctx.cmd_and_log(['cmd', '/E:on', '/V:on', '/C', vs_detect_bat_file.abspath()])
    lines = sout.splitlines()

    if not lines[0]:
        lines.pop(0)

    msvc_path = msvc_inc_path = msvc_lib_path = None
    for line in lines:
        if line.startswith('PATH='):
            path = line[5:]
            msvc_path = path.split(';')
        elif line.startswith('INCLUDE='):
            msvc_inc_path = [i for i in line[8:].split(';') if i]
        elif line.startswith('LIB='):
            msvc_lib_path = [i for i in line[4:].split(';') if i]

    if not msvc_path:
        raise Errors.WafError("Unable to determine msvc executable paths for {}".format(platform_name))
    if not msvc_inc_path:
        raise Errors.WafError("Unable to determine msvc include paths for {}".format(platform_name))
    if not msvc_lib_path:
        raise Errors.WafError("Unable to determine msvc lib paths for {}".format(platform_name))
    return msvc_path, msvc_inc_path, msvc_lib_path

SUPPORTED_WINSDK_MAJOR_VERSIONS = ['8.1', '10.0']


def query_win_sdk_install_path(reg_root, path, winsdk_version):
    """
    Query through reg edit for the windows sdk install path if possible

    :param reg_root:        The registry root (from _winreg, i.e. HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER, etc)
    :param path:            The path from the root from where to expect he key for the particular winsdk version
    :param winsdk_version:  The specific winsdk_version (only 8.1 and 10.0 are supported)
    :return: The installation path for the requested winsdk if possible, None if not a valid key or not found
    """

    if winsdk_version not in SUPPORTED_WINSDK_MAJOR_VERSIONS:
        raise Errors.WafError("Unsupported WinSDK version: {}. Only ({}) are supported.".format(winsdk_version, ','.join(SUPPORTED_WINSDK_MAJOR_VERSIONS)))

    reg_handle = None
    try:
        reg_handle = Utils.winreg.OpenKey(reg_root, '{}\\Microsoft\\Microsoft SDKs\\Windows\\v{}'.format(path, winsdk_version))
        installation_folder, _ = Utils.winreg.QueryValueEx(reg_handle, 'InstallationFolder')
        if not os.path.isdir(installation_folder):
            return None
        return installation_folder
    except WindowsError:
        return None
    finally:
        if reg_handle:
            Utils.winreg.CloseKey(reg_handle)


# Combination of registry roots and root of the subpaths to perform a search for the query various MS Installation registry entries
WIN_SDK_WINREG_ROOT_KEYS = [
    (Utils.winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\Wow6432Node'),
    (Utils.winreg.HKEY_CURRENT_USER,  'SOFTWARE\\Wow6432Node'),
    (Utils.winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE'),
    (Utils.winreg.HKEY_CURRENT_USER,  'SOFTWARE')
]


@conf
def get_msbuild_root(ctx, toolset_version):
    """
    Get the MSBuild root installed folder path from the registry

    :param ctx:                 Context
    :param toolset_version:     The toolset version
    :return: The root installation folder if found, None if not
    """
    for reg_root_and_path in WIN_SDK_WINREG_ROOT_KEYS:
        reg_root = reg_root_and_path[0]
        path = reg_root_and_path[1]
        try:
            reg_key = '{}\\Microsoft\\MSBuild\\ToolsVersions\\{}.0'.format(path, toolset_version)
            msbuild_root_regkey = Utils.winreg.OpenKey(reg_root, reg_key, 0, Utils.winreg.KEY_READ)
            root_path, value_type = Utils.winreg.QueryValueEx(msbuild_root_regkey, 'MSBuildToolsRoot')
            return root_path.encode('utf-8')
        except WindowsError:
            pass
    return None


@conf
def winreg_query_software_registry(ctx, query_func):
    """
    Query the potention registry jeys and paths and return the result of an input query function if matched
    :param ctx:         Context
    :param query_func:  The query function that takes in the registry root and path
    :return:  Result of the query
    """
    for reg_root_and_path in WIN_SDK_WINREG_ROOT_KEYS:
        try:
            result = query_func(reg_root_and_path[0], reg_root_and_path[1])
            if result:
                return result
        except WindowsError:
            pass
    return None


def get_winkit_10_0_install_path():
    """
    Detect the Windows Kit 10.0 Installation Path
    :return: The detected path if found, None if not
    """

    for reg_root_and_path in WIN_SDK_WINREG_ROOT_KEYS:
        win10_sdk_install_path = query_win_sdk_install_path(reg_root_and_path[0], reg_root_and_path[1], '10.0')
        if win10_sdk_install_path:
            return win10_sdk_install_path
    return None


def get_winkit_8_1_install_path():
    """
    Detect the Windows Kit 8.1 Installation Path
    :return: The detected path if found, None if not
    """

    for reg_root_and_path in WIN_SDK_WINREG_ROOT_KEYS:
        win8_sdk_install_path = query_win_sdk_install_path(reg_root_and_path[0], reg_root_and_path[1], '8.1')
        if win8_sdk_install_path:
            return win8_sdk_install_path
    return None


WIN10_SDK_CHECK_FILE = 'um\\winsdkver.h'


def get_winkit_10_0_versions():
    """
    Attempt to list all of the installed windows 10 SDK versions installed on this machine
    :return:    List of all windows 10 SDK installation folders
    """
    win10_sdk_versions = []

    # First locate the windows 10 SDK folder
    win10_sdk_install_path = get_winkit_10_0_install_path()
    if not win10_sdk_install_path:
        return win10_sdk_versions

    # Enumerate the subfolders under the 'include' folder of the win10 sdk folder
    win10_sdk_include_parent = os.path.join(win10_sdk_install_path, 'include')

    win10_sdk_contents = os.listdir(win10_sdk_include_parent)

    for win10_sdk_folder in win10_sdk_contents:
        check_file = os.path.join(win10_sdk_include_parent, win10_sdk_folder, WIN10_SDK_CHECK_FILE)
        if not os.path.isfile(check_file):
            continue
        win10_sdk_versions.append(win10_sdk_folder)

    win10_sdk_versions.sort(reverse=True)
    return win10_sdk_versions


def query_windows_sdk_install_path(reg_root, reg_path, version):
    """
    Query for a particular windows sdk version's installation path

    :param reg_root:    The registry root to conduct the query
    :param reg_path:    The registry path off of the root to query
    :param version:     The particular version to query
    :return: The windows sdk installed folder if found, None if not
    """

    microsoft_windows_sdk_hkey = None

    try:
        microsoft_windows_sdk_path = "{}\\Microsoft\\Microsoft SDKs\\Windows\\v{}".format(reg_path, version)
        microsoft_windows_sdk_hkey = Utils.winreg.OpenKey(reg_root, microsoft_windows_sdk_path)
        microsoft_sdks_folder, _ = Utils.winreg.QueryValueEx(microsoft_windows_sdk_hkey, 'InstallationFolder')
        return microsoft_sdks_folder
    except WindowsError:
        return None
    finally:
        if microsoft_windows_sdk_hkey:
            Utils.winreg.CloseKey(microsoft_windows_sdk_hkey)


def get_windows_sdk_install_path(version_key):
    """
    Search through all the possible known registry keys to look for a particular windows SDK installation path
    :param version_key:     The version key to search for
    :return:    The windows sdj install path if found, None if not
    """

    for reg_root_and_path in WIN_SDK_WINREG_ROOT_KEYS:
        windows_sdk_install_path = query_windows_sdk_install_path(reg_root_and_path[0],
                                                                  reg_root_and_path[1],
                                                                  version_key)
        if windows_sdk_install_path:
            return windows_sdk_install_path
    return None


def vs_installation_version_compare(left, right):
    """
    Simple helper function to do a version compare of vs installations
    :param left:    Left value to compare
    :param right:   Right value to compare
    :return: strcmp equivalent values
    """

    left_parts = left.split('.')
    right_parts = right.split('.')
    left_len = len(left_parts)
    right_len = len(right_parts)
    max_len = max(left_len, right_len)
    index = 0
    while index < max_len:
        if index > left_len:
            # Right is larger
            return -1
        elif index > right_len:
            # Left is larger
            return 1
        left_part_value = int(left_parts(index))
        right_part_value = int(right_parts(index))
        if left_part_value < right_part_value:
            # Right is larger
            return -1
        elif left_part_value > right_part_value:
            # Left is larger
            return 1
        index += 1
    # Both are equal
    return 0


@conf
def query_visual_studio_install_path(ctx, vs_version, vswhere_args):
    """
    Query for a particular visual studio install path from its version
    :param ctx:             Context
    :param version          Visual Studio major version to detect
    :param vswhere_args:    Additional arguments to pass to vswhere to filter the query
    :return:    The installation path of the installed VS product
    """

    vs_version_map = query_vswhere_versions(ctx, vswhere_args)
    if not vs_version_map:
        raise Errors.WafError("Unable to detect Visual Studio version '{}': vswhere args are '{}'".format(vs_version, vswhere_args))

    installation_version = vs_version_map.get('installationVersion', None)
    if not installation_version:
        raise Errors.WafError("Unable to detect Visual Studio version '{}': Bad installationVersion".format(vs_version))

    # From the installation path, determine the path to the expected 'vcvarsall.bat'
    path_visual_studio = vs_version_map['installationPath']
    if not os.path.isdir(path_visual_studio):
        raise Errors.WafError("Unable to detect Visual Studio version '{}': Queried installation path '{}' does not exist".format(vs_version, path_visual_studio))

    return path_visual_studio


@conf
def detect_visual_studio(ctx, platform_name, path_visual_studio, path_vcvars_all, winkit_version, vcvarsall_args):
    """
    Perform the detection of visual studio

    :param ctx:         The context
    :param platform_name:    The name of the waf platform to associate the visual studio to (on success)
    :param path_visual_studio:  Path to visual studio installation
    :param path_vcvars_all: Path to the vcvarsall.bat file within the visual studio installation path
    :param winkit_version: windows SDK version
    :param vcvarsall_args additional arguments to pass to vcvarsall.bat
    :return:    A result map that is processed downstream for additional processing
    """

    preprocessed_winkit_version = None
    winkit_install_path = None
    winkit_include_path = None
    windows_sdk_install_path = None
    additional_bin_path = None

    # Check the winkit version
    if winkit_version.startswith('10') or not winkit_version:
        # If its 10 or 10.0, then attempt to detect the newest version?
        win10_sdk_versions = get_winkit_10_0_versions()
        if win10_sdk_versions:
            for candidate_win10_version in win10_sdk_versions:
                if candidate_win10_version.startswith(winkit_version):
                    # The win10 sdk version is only viable if it has a include, lib, and bin folder
                    winkit_10_0_base_path = get_winkit_10_0_install_path()
                    winkit_10_0_include_path = os.path.join(winkit_10_0_base_path, 'Include', candidate_win10_version)
                    winkit_10_0_lib_path = os.path.join(winkit_10_0_base_path, 'Lib', candidate_win10_version)
                    winkit_10_0_bin_path = os.path.join(winkit_10_0_base_path, 'bin', candidate_win10_version, 'x64')
                    if os.path.isdir(winkit_10_0_include_path) and os.path.isdir(winkit_10_0_lib_path) and os.path.isdir(winkit_10_0_bin_path):
                        preprocessed_winkit_version = candidate_win10_version
                        winkit_install_path = winkit_10_0_base_path
                        additional_bin_path = winkit_10_0_bin_path
                        winkit_include_path = winkit_10_0_include_path
                        Logs.debug("Windows 10 SDK version ({}) found".format(preprocessed_winkit_version))
                        break

        if not preprocessed_winkit_version:
            print("Warning, unable to detect any windows 10 SDKs on this machine, attempting to locate windows 8.1 SDKs")
            if get_winkit_8_1_install_path() is not None:
                preprocessed_winkit_version = '8.1'
                print("Falling back on detected windows 8.1 SDK")
    elif winkit_version == '8.1':
        winkit_8_1_sdk_path = get_winkit_8_1_install_path()
        if winkit_8_1_sdk_path:
            winkit_install_path = winkit_8_1_sdk_path

            win_8_1_bin_path = os.path.join(winkit_install_path, 'bin', 'x64')
            if os.path.isdir(win_8_1_bin_path):
                additional_bin_path = win_8_1_bin_path

            win_8_1_include_path = os.path.join(winkit_install_path, 'Include')
            if os.path.isdir(win_8_1_include_path):
                winkit_include_path = win_8_1_include_path

            preprocessed_winkit_version = '8.1'
    else:
        raise Errors.WafError("Unsupported windows SDK version ({})".format(winkit_version))

    if not preprocessed_winkit_version:
        raise Errors.WafError("Unable to locate any valid windows SDK on this host")

    if preprocessed_winkit_version=='8.1':
        windows_sdk_install_path = get_windows_sdk_install_path('8.1')
    else:
        windows_sdk_install_path = get_windows_sdk_install_path('10.0')

    # Check for a cache of vs environments
    cache_path = os.path.join(ctx.bldnode.abspath(), BINTEMP_CACHE_TOOLS)
    if not os.path.isdir(cache_path):
        os.makedirs(cache_path)
    vc_env_cache_path = os.path.join(cache_path, 'vs_env_cache.json')
    if os.path.exists(vc_env_cache_path):
        vc_env_cache = parse_json_file(vc_env_cache_path)
    else:
        vc_env_cache = {}

    vs_env_key = '{}/{}'.format(platform_name, preprocessed_winkit_version)
    if vs_env_key in vc_env_cache:
        try:
            # Read the cached values first
            vs_env_data = vc_env_cache[vs_env_key]
            msvc_path = vs_env_data['msvc_path']
            msvc_inc_path = vs_env_data['msvc_inc_path']
            msvc_lib_path = vs_env_data['msvc_lib_path']
            cxx = vs_env_data['cxx']
            linker = vs_env_data['linker']
            winrc = vs_env_data['winrc']
            mt = vs_env_data['mt']
            ar = vs_env_data['ar']

            for check_path in [cxx, linker, winrc, mt, ar]:
                if not os.path.isfile(check_path):
                    raise ValueError("Cannot locate ''".format(check_path))

            result = {
                "INCLUDES": msvc_inc_path,
                "PATH": msvc_path,
                "LIBPATH": msvc_lib_path,
                "CC": cxx,
                "CXX": cxx,
                "LINK": linker,
                "LINK_CC": linker,
                "LINK_CXX": linker,
                "VS_INSTALLATION_PATH": path_visual_studio,
                "WINRC": winrc,
                "MT": mt,
                "AR": ar,
            }

            if windows_sdk_install_path:
                result['WINDOWS_SDK_FOLDER'] = windows_sdk_install_path
            if winkit_install_path:
                result['WINKIT_INSTALL_PATH'] = winkit_install_path
            if winkit_include_path:
                result['WINDOWS_KITS_INCLUDE_PATH'] = winkit_include_path

            return result

        except ValueError:
            pass

    msvc_path, msvc_inc_path, msvc_lib_path = ctx.query_vcvars_all(platform_name=platform_name,
                                                                   vcvars_path=path_vcvars_all,
                                                                   compiler_version='x64',
                                                                   winkit_version=preprocessed_winkit_version,
                                                                   vcvarsall_args=vcvarsall_args)
    if additional_bin_path and additional_bin_path not in msvc_path:
        msvc_path = [additional_bin_path] + msvc_path

    cxx = ctx.find_program(VC_COMPILER_NAME, path_list=msvc_path, silent_output=True, var='CXX')
    linker = ctx.find_program(VC_LINKER_NAME, path_list=msvc_path, silent_output=True, var='LINK')
    winrc = ctx.find_program(VC_RC_NAME, path_list=msvc_path, silent_output=True, var='WINRC')
    mt = ctx.find_program(VC_MT_NAME, path_list=msvc_path, silent_output=True, var='MT')
    ar = ctx.find_program(VC_AROOL_NAME, path_list=msvc_path, silent_output=True, var='AR')

    vs_env_data = {
        'msvc_path': msvc_path,
        'msvc_inc_path': msvc_inc_path,
        'msvc_lib_path': msvc_lib_path,
        'cxx': cxx,
        'linker': linker,
        'winrc': winrc,
        'mt': mt,
        'ar': ar
    }
    vc_env_cache[vs_env_key] = vs_env_data

    write_json_file(vc_env_cache, vc_env_cache_path)

    result = {
        "INCLUDES": msvc_inc_path,
        "PATH": msvc_path,
        "LIBPATH": msvc_lib_path,
        "CC": cxx,
        "CXX": cxx,
        "LINK": linker,
        "LINK_CC": linker,
        "LINK_CXX": linker,
        "VS_INSTALLATION_PATH": path_visual_studio,
        "WINRC": winrc,
        "MT": mt,
        "AR": ar,
    }

    return result


@conf
def apply_dev_studio_environment(ctx, dev_studio_env_map, env_keys_to_apply=None):
    """
    From the MS Dev Studio environment result map, apply the values of specific keys to the current context's environment

    :param ctx:                 The context to apply to
    :param dev_studio_env_map:  The map to read the key-value pair
    :param env_keys_to_apply:   Keys to restrict the application to the environment if provided, otherwise apply all keys from the map
    """
    for ds_key, ds_value in list(dev_studio_env_map.items()):
        if env_keys_to_apply and ds_key not in env_keys_to_apply:
            continue
        if isinstance(ds_value, list):
            ctx.env[ds_key] = ds_value + ctx.env[ds_key]
        elif isinstance(ds_value, str):
            ctx.env[ds_key] = ds_value
        elif isinstance(ds_value, bytes):
            ctx.env[ds_key] = ds_value.decode(sys.stdout.encoding or 'iso8859-1', 'replace')
        else:
            raise Errors.WafError("Invalid dev studio env type for field '{}'. Expected a string or list.".format(ds_key))

