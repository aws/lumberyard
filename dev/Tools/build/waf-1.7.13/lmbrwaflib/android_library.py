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

import os, re, zipfile

import xml.etree.ElementTree as ET

from cry_utils import append_to_unique_list

from waflib import Context, Errors, Logs, Node, Utils
from waflib.Configure import conf
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waflib.TaskGen import feature


def get_package_name(android_manifest):
    xml_tree = ET.parse(android_manifest)
    xml_node = xml_tree.get_root()

    raw_attributes = getattr(xml_node, 'attrib', None)
    if not raw_attributes:
        Logs.debug('android_library: No attributes found in element {}'.format(xml_node.tag))
        return None

    return raw_attributes.get('package', None)


class fake_jar(Task):
    '''
    Dummy primary Java Archive Resource task for an Android Archive Resource
    '''
    def runnable_status(self):
        for tsk in self.run_after:
            if not tsk.hasrun:
                return ASK_LATER

        for output in self.outputs:
            output.sig = Utils.h_file(output.abspath())

        return SKIP_ME


class fake_aar(Task):
    '''
    Dummy Android Archive Resource task
    '''
    def runnable_status(self):
        for tsk in self.run_after:
            if not tsk.hasrun:
                return ASK_LATER

        for output in self.outputs:
            output.sig = Utils.h_file(output.abspath())

        return SKIP_ME


class android_manifest_merger(Task):
    '''
    Merges the input manifests into the "main" manifest specifed in the task generator 
    '''
    color = 'PINK'
    run_str = '${JAVA} -cp ${MANIFEST_MERGER_CLASSPATH} ${MANIFEST_MERGER_ENTRY_POINT} --main ${MAIN_MANIFEST} --libs ${LIBRARY_MANIFESTS} --out ${TGT}'

    def runnable_status(self):
        result = super(android_manifest_merger, self).runnable_status()
        if result == SKIP_ME:
            for output in self.outputs:
                if not os.path.isfile(output.abspath()):
                    Logs.debug('android_library: Output manifest not found')
                    return RUN_ME

                output.sig = Utils.h_file(output.abspath())

        return result


@conf
def read_aar(conf, name, paths, is_android_support_lib = False, android_studio_name = None):
    '''
    Read an Android Archive Resource, enabling its use as a local Android library. Will trigger a rebuild if the file changes
    :param conf:    The Context
    :param name:    Name of the task
    :param paths:   Possible locations on disk the library could be
    :param is_android_support_lib: [Optional] Special flag for Android support libraries so the version can be auto detected 
                                    based on the specified Android API level used.  Default is False
    :param android_studio_name:    [Optional] Override the name used in the Android Studio project generation.  This is mostly
                                    used for AARs that come with the Android SDK as they are sourced from a repository, which
                                    requires a special naming convention. Default is none, in which the name will be used

    def build(bld):
        bld.read_aar(
            name = 'play-services-games-11.0.2', 
            paths = '/Developer/Android/sdk/extras/google/m2repository/com/google/android/gms/play-services-games/11.0.2',
            android_studio_name = 'com.google.android.gms:play-services-games:11.0.2
        )

        bld.read_aar(
            name = 'support-v4', 
            is_android_support_lib = True,
            paths = '/Developer/Android/sdk/extras/android/m2repository/com/android/support/support-v4', # root path to the versions
        )

        bld.AndroidAPK(project_name = 'AwesomeGame', use = [ 'play-services-games-11.0.2', 'support-v4' ])
    '''
    if isinstance(paths, str):
        paths = [ paths ]

    return conf(name = name, features = 'fake_aar', lib_paths = paths, is_android_support_lib = is_android_support_lib, android_studio_name = android_studio_name)


@feature('fake_aar')
def process_aar(self):
    """
    Find the Android library and unpack it so it's resources can be used by other modules
    """
    if self.bld.env['PLATFORM'] not in ('android_armv7_clang', 'android_armv7_gcc', 'project_generator'):
        Logs.debug('android_library: Skipping the reading of the aar')
        return

    Utils.def_attrs(
        self,

        manifest = None,
        package = '',

        classpath = [],

        native_libs = [],

        aapt_assets = None,
        aapt_resources = None,
    )

    lib_node = None
    search_paths = []

    if self.is_android_support_lib:
        android_api_level = str(self.env['ANDROID_SDK_VERSION_NUMBER'])
        Logs.debug('android_library: Searching for support library - %s - built with API %s', self.name, android_api_level)

        for path in self.lib_paths:
            if os.path.exists(path):
                entries = os.listdir(path)
                Logs.debug('android_library: All API versions installed {}'.format(entries))

                api_versions = sorted([ entry for entry in entries if entry.startswith(android_api_level) ])
                Logs.debug('android_library: Found versions {}'.format(api_versions))
                highest_useable_version = api_versions[-1]

                search_paths.append(os.path.join(path, highest_useable_version))

                self.android_studio_name = 'com.android.support:{}:{}'.format(self.name, highest_useable_version)
                lib_name = '{}-{}.aar'.format(self.name, highest_useable_version)
                break
        else:
            raise Errors.WafError('Unable to detect a valid useable version for Android support library  %r' % self.name)

    else:
        lib_name = '{}.aar'.format(self.name)
        search_paths = self.lib_paths + [ self.path ]

    for path in search_paths:
        Logs.debug('android_library: Searching path {}'.format(path))

        if not isinstance(path, Node.Node):
            path = self.bld.root.find_node(path) or self.path.find_node(path)
            if not path:
                Logs.debug('android_library: Unable to find node for path')
                continue

        lib_node = path.find_node(lib_name)
        if lib_node:
            lib_node.sig = Utils.h_file(lib_node.abspath())
            break

    else:
        raise Errors.WafError('Could not find Android library %r' % self.name)

    android_cache = self.bld.get_android_cache_node()

    extraction_node = android_cache.make_node([ 'aar', self.name ])
    if os.path.exists(extraction_node.abspath()):
        extraction_node.delete()
    extraction_node.mkdir()

    aar_file = zipfile.ZipFile(file = lib_node.abspath())
    aar_file.extractall(path = extraction_node.abspath())
    Logs.debug('android_library: AAR contents = {}'.format(aar_file.namelist()))

    # required entries from the aar
    main_jar_file = extraction_node.find_node('classes.jar')
    if not main_jar_file:
        self.bld.fatal('[ERROR] Unable to find the required classes.jar from {}'.format(lib_name))

    self.manifest = extraction_node.find_node('AndroidManifest.xml')
    if not self.manifest:
        self.bld.fatal('[ERROR] Unable to find the required AndroidManifest.xml from {}'.format(lib_name))

    self.package = get_package_name(self.manifest.abspath())
    if not self.package:
        self.bld.fatal('[ERROR] Failed to extract the package name from AndroidManifest.xml in {}'.format(lib_name))

    self.aapt_resources = extraction_node.find_dir('res')
    if not self.aapt_resources:
        self.bld.fatal('[ERROR] Unable to find the required resources directory - res - from {}'.format(lib_name))

    # optional entries from the aar
    self.aapt_assets = extraction_node.find_dir('assets')

    java_libs = extraction_node.find_dir('libs')
    if java_libs:
        self.classpath = java_libs.ant_glob('**/*.jar')

    native_lib_path = 'jni/{}'.format(self.bld.env['ANDROID_ARCH'])
    native_libs = extraction_node.find_dir(native_lib_path)
    if native_libs:
        self.native_libs_root = native_libs
        self.native_libs = native_libs.ant_glob('**/*.so')

    # create the fake tasks
    self.jar_task = self.create_task('fake_jar', [], main_jar_file)
    self.aar_task = self.create_task('fake_aar', [], lib_node)

    # task chaining
    self.aar_task.set_run_after(self.jar_task)

