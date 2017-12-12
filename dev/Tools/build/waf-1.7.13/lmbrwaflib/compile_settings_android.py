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
from waflib.Configure import conf
from cry_utils import append_to_unique_list


################################################################
@conf
def load_android_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all android configurations
    """

    env = conf.env
    ndk_root = env['ANDROID_NDK_HOME']

    # common settings for all android builds
    defines = [
        '_LINUX',
        'LINUX',
        'ANDROID',
        'MOBILE',
        '_HAS_C9X',
        'ENABLE_TYPE_INFO',

        'NDK_REV_MAJOR={}'.format(env['ANDROID_NDK_REV_MAJOR']),
        'NDK_REV_MINOR={}'.format(env['ANDROID_NDK_REV_MINOR']),
    ]

    if conf.is_using_android_unified_headers():
        defines += [
            '__ANDROID_API__={}'.format(env['ANDROID_NDK_PLATFORM_NUMBER']),
        ]

    append_to_unique_list(env['DEFINES'], defines)

    env['LIB'] += [
        'android',              # android library
        'c',                    # c library for android
        'log',                  # log library for android
        'dl',                   # dynamic library
    ]

    env['LINKFLAGS'] += [
        '-rdynamic',            # add ALL symbols to the dynamic symbol table
        '-Wl,--no-undefined'    # tell the gcc linker to fail if it finds undefined references
    ]

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

    # appt settings
    env['APPT_RESOURCE_ST'] = [ '-S' ]
    env['APPT_RESOURCES'] = []

    env['APPT_INLC_ST'] = [ '-I' ]
    env['APPT_INCLUDES'] = [ android_jar ]
    env['APP_PACKAGE_FLAGS'] = [ ]

    # apk packaging settings
    env['ANDROID_MANIFEST'] = ''
    env['ANDROID_DEBUG_MODE'] = ''

    # jarsigner settings
    env['KEYSTORE_ALIAS'] = conf.get_android_env_keystore_alias()
    env['KEYSTORE'] = conf.get_android_env_keystore_path()


################################################################
@conf
def load_debug_android_settings(conf):
    """
    Setup all compiler and linker settings shared over all android configurations
    for the "debug" configuration
    """

    conf.load_android_common_settings()
    env = conf.env

    common_flags = [
        '-gdwarf-2',            # DWARF 2 debugging information
    ]

    env['CFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += common_flags[:]

    env['LINKFLAGS'] += [
         '-Wl,--build-id'       # Android Studio needs the libraries to have an id in order to match them with what's running on the device.
    ]

    env['ANDROID_DEBUG_MODE'] = '--debug-mode'
    env['JAVACFLAGS'] += [
        '-g'        # enable all debugging information
    ]


################################################################
@conf
def load_profile_android_settings(conf):
    """
    Setup all compiler and linker settings shared over all android configurations
    for the "profile" configuration
    """

    conf.load_android_common_settings()
    env = conf.env

    common_flags = [
        '-g',           # debugging information
        '-gdwarf-2'     # DWARF 2 debugging information
    ]

    env['CFLAGS'] += common_flags[:]
    env['CXXFLAGS'] += common_flags[:]

    env['LINKFLAGS'] += [
         '-Wl,--build-id'   # Android Studio needs the libraries to have an id in order to match them with what's running on the device.
    ]

    env['ANDROID_DEBUG_MODE'] = '--debug-mode'
    env['JAVACFLAGS'] += [
        '-g'        # enable all debugging information
    ]


################################################################
@conf
def load_performance_android_settings(conf):
    """
    Setup all compiler and linker settings shared over all android configurations
    for the "performance" configuration
    """

    conf.load_android_common_settings()

    env = conf.env
    env['JAVACFLAGS'] += [
        '-g:none'           # disable all debugging information
    ]


################################################################
@conf
def load_release_android_settings(conf):
    """
    Setup all compiler and linker settings shared over all android configurations
    for the "release" configuration
    """

    conf.load_android_common_settings()

    env = conf.env
    env['JAVACFLAGS'] += [
        '-g:none'           # disable all debugging information
    ]

