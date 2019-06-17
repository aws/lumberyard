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
import os
from waflib import Logs
from waflib.Configure import conf
from cry_utils import append_to_unique_list
from lumberyard import deprecated


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

