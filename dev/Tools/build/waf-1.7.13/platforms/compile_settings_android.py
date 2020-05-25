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

# waflib imports
from waflib import Logs
from waflib.Configure import conf

# lmbrwaflib imports
from lmbrwaflib import lumberyard
from lmbrwaflib.cry_utils import append_to_unique_list


@conf
def load_android_toolchains(ctx, search_paths, CC, CXX, AR, STRIP, **addition_toolchains):
    """
    Loads the native toolchains from the Android NDK
    """
    try:
        ctx.find_program(CC, var = 'CC', path_list = search_paths, silent_output = True)
        ctx.find_program(CXX, var = 'CXX', path_list = search_paths, silent_output = True)
        ctx.find_program(AR, var = 'AR', path_list = search_paths, silent_output = True)

        # for debug symbol stripping
        ctx.find_program(STRIP, var = 'STRIP', path_list = search_paths, silent_output = True)

        # optional linker override
        if 'LINK_CC' in addition_toolchains and 'LINK_CXX' in addition_toolchains:
            ctx.find_program(addition_toolchains['LINK_CC'], var = 'LINK_CC', path_list = search_paths, silent_output = True)
            ctx.find_program(addition_toolchains['LINK_CXX'], var = 'LINK_CXX', path_list = search_paths, silent_output = True)
        else:
            ctx.env['LINK_CC'] = ctx.env['CC']
            ctx.env['LINK_CXX'] = ctx.env['CXX']

        ctx.env['LINK'] = ctx.env['LINK_CC']

        # common cc settings
        ctx.cc_load_tools()
        ctx.cc_add_flags()

        # common cxx settings
        ctx.cxx_load_tools()
        ctx.cxx_add_flags()

        # common link settings
        ctx.link_add_flags()
    except:
        Logs.error('[ERROR] Failed to find the Android NDK standalone toolchain(s) in search path %s' % search_paths)
        return False

    return True


@conf
def load_android_tools(ctx):
    """
    Loads the necessary build tools from the Android SDK
    """
    android_sdk_home = ctx.env['ANDROID_SDK_HOME']

    build_tools_version = ctx.get_android_build_tools_version()
    build_tools_dir = os.path.join(android_sdk_home, 'build-tools', build_tools_version)

    try:
        ctx.find_program('aapt', var = 'AAPT', path_list = [ build_tools_dir ], silent_output = True)
        ctx.find_program('aidl', var = 'AIDL', path_list = [ build_tools_dir ], silent_output = True)
        ctx.find_program('dx', var = 'DX', path_list = [ build_tools_dir ], silent_output = True)
        ctx.find_program('zipalign', var = 'ZIPALIGN', path_list = [ build_tools_dir ], silent_output = True)
    except:
        Logs.error('[ERROR] The desired Android SDK build-tools version - {} - appears to be incomplete.  Please use Android SDK Manager to validate the build-tools version installation '
                         'or change BUILD_TOOLS_VER in _WAF_/android/android_settings.json to either a version installed or "latest" and run the configure command again.'.format(build_tools_version))
        return False

    return True


@conf
def load_android_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all android configurations
    """

    env = conf.env
    ndk_root = env['ANDROID_NDK_HOME']

    defines = []

    if env['ANDROID_NDK_REV_MAJOR'] < 19:
        defines += [
            '__ANDROID_API__={}'.format(env['ANDROID_NDK_PLATFORM_NUMBER']),
        ]

    append_to_unique_list(env['DEFINES'], defines)

    # Pattern to transform outputs
    env['cprogram_PATTERN'] = env['cxxprogram_PATTERN'] = '%s'
    env['cshlib_PATTERN'] = env['cxxshlib_PATTERN'] = 'lib%s.so'
    env['cstlib_PATTERN'] = env['cxxstlib_PATTERN'] = 'lib%s.a'

    env['RPATH_ST'] = '-Wl,-rpath,%s'
    env['SONAME_ST'] = '-Wl,-soname,%s'   # sets the DT_SONAME field in the shared object, used for ELF object loading

    # frameworks aren't supported on Android, disable it
    env['FRAMEWORK'] = []
    env['FRAMEWORK_ST'] = ''
    env['FRAMEWORKPATH'] = []
    env['FRAMEWORKPATH_ST'] = ''

    # java settings
    env['JAVA_VERSION'] = '1.7'
    env['CLASSPATH'] = []

    platform = os.path.join(env['ANDROID_SDK_HOME'], 'platforms', env['ANDROID_SDK_VERSION'])
    android_jar = os.path.join(platform, 'android.jar')

    env['JAVACFLAGS'] = [
        '-encoding', 'UTF-8',
        '-bootclasspath', android_jar,
        '-target', env['JAVA_VERSION'],
    ]

    # android interface processing
    env['AIDL_PREPROC_ST'] = '-p%s'
    env['AIDL_PREPROCESSES'] = [ os.path.join(platform, 'framework.aidl') ]

    # aapt settings
    env['AAPT_ASSETS_ST'] = [ '-A' ]
    env['AAPT_ASSETS'] = []

    env['AAPT_RESOURCE_ST'] = [ '-S' ]
    env['AAPT_RESOURCES'] = []

    env['AAPT_INLC_ST'] = [ '-I' ]
    env['AAPT_INCLUDES'] = [ android_jar ]

    env['AAPT_PACKAGE_FLAGS'] = [ '--auto-add-overlay' ]

    # apk packaging settings
    env['ANDROID_MANIFEST'] = ''
    env['ANDROID_DEBUG_MODE'] = ''

    # manifest merger settings
    tools_path = os.path.join(env['ANDROID_SDK_HOME'], 'tools', 'lib')
    tools_contents = os.listdir(tools_path)
    tools_jars = [ entry for entry in tools_contents if entry.lower().endswith('.jar') ]

    manifest_merger_lib_names = [
        # entry point for the merger
        'manifest-merger',

        # dependent libs
        'sdk-common',
        'common'
    ]

    manifest_merger_libs = []
    for jar in tools_jars:
        if any(lib_name for lib_name in manifest_merger_lib_names if jar.lower().startswith(lib_name)):
            manifest_merger_libs.append(jar)

    if len(manifest_merger_libs) < len(manifest_merger_lib_names):
        conf.fatal('[ERROR] Failed to find the required file(s) for the Manifest Merger.  Please use the Android SDK Manager to update to the latest SDK Tools version and run the configure command again.')

    env['MANIFEST_MERGER_CLASSPATH'] = os.pathsep.join([ os.path.join(tools_path, jar_file) for jar_file in manifest_merger_libs ])

    # zipalign settings
    env['ZIPALIGN_SIZE'] = '4' # alignment in bytes, e.g. '4' provides 32-bit alignment (has to be a string)

    # jarsigner settings
    env['KEYSTORE_ALIAS'] = conf.get_android_env_keystore_alias()
    env['KEYSTORE'] = conf.get_android_env_keystore_path()


@lumberyard.multi_conf
def generate_ib_profile_tool_elements(ctx):
    android_tool_elements = [
        '<Tool Filename="arm-linux-androideabi-gcc" AllowRemoteIf="-c" AllowIntercept="false" DeriveCaptionFrom="lastparam" AllowRestartOnLocal="false"/>',
        '<Tool Filename="arm-linux-androideabi-g++" AllowRemoteIf="-c" AllowIntercept="false" DeriveCaptionFrom="lastparam" AllowRestartOnLocal="false"/>',
        # The Android deploy command uses 'adb shell' to check files on the device and will return non 0 exit codes if the files don't exit.  XGConsole will flag those processes as
        # failures and since we are gracefully handing those cases, WAF will continue to execute till it exits naturally (we don't use /stoponerror).  In most cases, this is causing
        # false build failures even though the deploy command completes sucessfully.  Whitelist the known return codes we handle internally.
        #
        '<Tool Filename="adb" AllowRemote="false" AllowIntercept="false" SuccessExitCodes="-1,0,1"/>'
    ]
    return android_tool_elements


def _load_android_settings(ctx):
    """ Helper function for loading the global android settings """

    if hasattr(ctx, 'android_settings'):
        return

    settings_files = list()
    for root_node in ctx._get_settings_search_roots():
        settings_node = root_node.make_node(['_WAF_', 'android', 'android_settings.json'])
        if os.path.exists(settings_node.abspath()):
            settings_files.append(settings_node)

    ctx.android_settings = dict()
    for settings_file in settings_files:
        try:
            ctx.android_settings.update(ctx.parse_json_file(settings_file))
        except Exception as e:
            ctx.cry_file_error(str(e), settings_file.abspath())


def _get_android_setting(ctx, setting, default_value = None):
    """" Helper function for getting android settings """
    _load_android_settings(ctx)

    return ctx.android_settings.get(setting, default_value)

@conf
def get_android_dev_keystore_alias(conf):
    return _get_android_setting(conf, 'DEV_KEYSTORE_ALIAS')

@conf
def get_android_dev_keystore_path(conf):
    local_path = _get_android_setting(conf, 'DEV_KEYSTORE')
    debug_ks_node = conf.engine_node.make_node(local_path)
    return debug_ks_node.abspath()

@conf
def get_android_distro_keystore_alias(conf):
    return _get_android_setting(conf, 'DISTRO_KEYSTORE_ALIAS')

@conf
def get_android_distro_keystore_path(conf):
    local_path = _get_android_setting(conf, 'DISTRO_KEYSTORE')
    release_ks_node = conf.engine_node.make_node(local_path)
    return release_ks_node.abspath()

@conf
def get_android_build_environment(conf):
    env = _get_android_setting(conf, 'BUILD_ENVIRONMENT')
    if env == 'Development' or env == 'Distribution':
        return env
    else:
        Logs.fatal('[ERROR] Invalid android build environment, valid environments are: Development and Distribution')

@conf
def get_android_env_keystore_alias(conf):
    env = conf.get_android_build_environment()
    if env == 'Development':
        return conf.get_android_dev_keystore_alias()
    elif env == 'Distribution':
        return conf.get_android_distro_keystore_alias()

@conf
def get_android_env_keystore_path(conf):
    env = conf.get_android_build_environment()
    if env == 'Development':
        return conf.get_android_dev_keystore_path()
    elif env == 'Distribution':
        return conf.get_android_distro_keystore_path()

@conf
def get_android_build_tools_version(conf):
    """
    Get the version of build-tools to use for Android APK packaging process.  Also
    sets the 'buildToolsVersion' when generating the Android Studio gradle project.
    The following is require in order for validation and use of "latest" value.

    def configure(conf):
        conf.load('android')
    """
    build_tools_version = conf.env['ANDROID_BUILD_TOOLS_VER']

    if not build_tools_version:
        build_tools_version = _get_android_setting(conf, 'BUILD_TOOLS_VER')

    return build_tools_version

@conf
def get_android_sdk_version(conf):
    """
    Gets the desired Android API version used when building the Java code,
    must be equal to or greater than the ndk_platform.  The following is
    required for the validation to work and use of "latest" value.

    def configure(conf):
        conf.load('android')
    """
    sdk_version = conf.env['ANDROID_SDK_VERSION']

    if not sdk_version:
        sdk_version = _get_android_setting(conf, 'SDK_VERSION')

    return sdk_version

@conf
def get_android_ndk_platform(conf):
    """
    Gets the desired Android API version used when building the native code,
    must be equal to or less than the sdk_version.  If not specified, the
    specified sdk version, or closest match, will be used.  The following is
    required for the auto-detection and validation to work.

    def configure(conf):
        conf.load('android')
    """
    ndk_platform = conf.env['ANDROID_NDK_PLATFORM']

    if not ndk_platform:
        ndk_platform = _get_android_setting(conf, 'NDK_PLATFORM')

    return ndk_platform

@conf
def get_android_project_relative_path(self):
    return '{}/{}'.format(self.options.android_studio_project_folder, self.options.android_studio_project_name)

@conf
def get_android_project_absolute_path(self):
    return self.path.make_node([ self.options.android_studio_project_folder, self.options.android_studio_project_name ]).abspath()

@conf
def get_android_patched_libraries_relative_path(self):
    return '{}/{}'.format(self.get_android_project_relative_path(), 'AndroidLibraries')