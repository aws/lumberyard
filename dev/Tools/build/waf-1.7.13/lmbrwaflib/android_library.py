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

import errno, os, re, string, urllib, urllib2, zipfile

from socket import error as SocketError
from ssl import SSLEOFError as SSLEOFError

import xml.etree.ElementTree as ET

from cry_utils import append_to_unique_list

from waflib import Context, Errors, Logs, Node, Utils
from waflib.Configure import conf
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waflib.TaskGen import feature


################################################################
#                     Defaults                                 #
ANDROID_SDK_LOCAL_REPOS = [
    '${ANDROID_SDK_HOME}/extras/android/m2repository',
    '${ANDROID_SDK_HOME}/extras/google/m2repository',
    '${ANDROID_SDK_HOME}/extras/m2repository',
    '${THIRD_PARTY}/android-sdk/1.0-az/google/m2repository',
]

GOOGLE_MAIN_MAVEN_REPO = 'maven.google.com'
GOOGLE_MAIN_MAVEN_REPO_INDEX = {}

GOOGLE_BINTRAY_MAVEN_REOPS = [
    'google.bintray.com/play-billing',
    'google.bintray.com/googlevr',
]

PROTOCOL = None
#                                                              #
################################################################


def is_xml_elem_valid(xml_elem):
    '''
    FutureWarning safe way of checking to see if an XML element is valid
    '''
    if xml_elem is not None:
        return True
    else:
        return False


def get_package_name(android_manifest):
    xml_tree = ET.parse(android_manifest)
    xml_node = xml_tree.getroot()

    raw_attributes = getattr(xml_node, 'attrib', None)
    if not raw_attributes:
        Logs.debug('android_library: No attributes found in element {}'.format(xml_node.tag))
        return None

    return raw_attributes.get('package', None)


def attempt_to_open_url(url):
    try:
        return urllib2.urlopen(url, timeout = 1)

    except Exception as error:
        Logs.debug('android_library: Failed to open URL %s', url)
        Logs.debug('android_library: Exception = %s', error)

        # known errors related to an SSL issue on macOS
        if isinstance(error, urllib2.URLError):
            reason = error.reason
            if isinstance(reason, SSLEOFError) or (isinstance(reason, SocketError) and reason.errno == errno.ECONNRESET):
                Logs.debug('android_library: If url is using HTTPS, try running the command with --android-maven-force-http-requests=True, '
                            'or setting android_maven_force_http=True in _WAF_/user_settings.options.')

    return None


def build_google_main_maven_repo_index():
    '''
    Connects to the Google's main Maven repository and creates a local map of all libs currently hosted there
    '''

    global GOOGLE_MAIN_MAVEN_REPO_INDEX
    if GOOGLE_MAIN_MAVEN_REPO_INDEX:
        return

    global PROTOCOL
    master_root_url = '{}://{}'.format(PROTOCOL, GOOGLE_MAIN_MAVEN_REPO)

    master_index_url = '/'.join([ master_root_url, 'master-index.xml' ])
    master_repo = attempt_to_open_url(master_index_url)
    if not master_repo:
        Logs.error('[ERROR] Failed to connect to {}.  Unable to access to Google\'s main Maven repository at this time.'.format(master_index_url))
        return

    data = master_repo.read()
    if not data:
        Logs.error('[ERROR] Failed to retrive data from {}.  Unable to access to Google\'s main Maven repository at this time.'.format(master_index_url))
        return

    master_index = ET.fromstring(data)
    if not is_xml_elem_valid(master_index):
        Logs.error('[ERROR] Data retrived from {} is malformed.  Unable to access to Google\'s main Maven repository at this time.'.format(master_index_url))
        return

    for group in master_index:
        Logs.debug('android_library: Adding group %s to the main maven repo index', group.tag)

        group_index_url = '/'.join([ master_root_url ] + group.tag.split('.') + [ 'group-index.xml' ])

        group_index = attempt_to_open_url(group_index_url)
        if not group_index:
            Logs.warn('[WARN] Failed to connect to {}.  Access to Google\'s main Maven repository may be incomplete.'.format(group_index_url))
            continue

        data = group_index.read()
        if not data:
            Logs.warn('[WARN] Failed to retrive data from {}.  Access to Google\'s main Maven repository may be incomplete.'.format(group_index_url))
            continue

        group_libraries = {}

        group_index = ET.fromstring(data)
        for lib in group_index:
            versions = lib.attrib.get('versions', None)
            if not versions:
                Logs.warn('[WARN] No found versions for library {} in group {}.  Skipping'.format(lib.tag, group))
                continue

            Logs.debug('android_library:    -> Adding library %s with version(s) %s', lib.tag, versions)
            group_libraries[lib.tag] = versions.split(',')

        GOOGLE_MAIN_MAVEN_REPO_INDEX[group.tag] = group_libraries


def get_bintray_version_info(url_root):
    '''
    Connects to a Bintray Maven repository and gets the latest version and all versions currently hosted here
    '''
    def _log_error(message):
        Logs.error('[ERROR] {}.  Unable to access this repository at this time.'.format(message))

    bintray_metadata_url = '/'.join([ url_root, 'maven-metadata.xml' ])

    bintray_metadata = attempt_to_open_url(bintray_metadata_url)
    if not bintray_metadata:
        return None, []

    data = bintray_metadata.read()
    if not data:
        _log_error('Failed to retrive data from {}'.format(bintray_metadata_url))
        return None, []

    metadata_root = ET.fromstring(data)
    if not is_xml_elem_valid(metadata_root):
        _log_error('Data retrived from {} is malformed'.format(bintray_metadata_url))
        return None, []

    versioning_node = metadata_root.find('versioning')
    if not is_xml_elem_valid(versioning_node):
        _log_error('Data retrived from {} is does not contain versioning information'.format(bintray_metadata_url))
        return None, []


    all_versions = []
    versions_node = versioning_node.find('versions')
    for ver_entry in versions_node:
        if ver_entry.tag == 'version':
            all_versions.append(ver_entry.text)

    if not all_versions:
        _log_error('Data retrived from {} is does not contain master version list'.format(bintray_metadata_url))
        return None, []


    latest_version = None
    latest_version_node = versioning_node.find('latest')
    if is_xml_elem_valid(latest_version_node):
        latest_version = latest_version_node.text
    else:
        Logs.warn('[WARN] Data retrived from {} is does not contain a latest version entry.  Auto-detecting latest from master version list'.format(bintray_metadata_url))
        latest_version = all_versions[-1] # given the 2 bintray repos used as an example, it's safe to use the last entry from the master version list

    return latest_version, all_versions


def search_maven_repos(ctx, name, group, version):
    '''
    Searches all known maven repositories (local, main and bintray) for the exact library or closest match based on the inputs
    '''

    group_path = group.replace('.', '/')

    partial_version = False
    if version and '+' in version:
        Logs.warn('[WARN] It is not recommended to use "+" in version numbers.  '
                  'This will lead to unpredictable results due to the version silently changing.  '
                  'Found while processing {}'.format(name))
        partial_version = True


    def _filter_versions(versions_list):
        if partial_version:
            base_version = version.split('+')[0]
            valid_versions = [ ver for ver in versions_list if ver.startswith(base_version) ]

        # the support lib versions are based on API built against
        elif group == 'com.android.support' and 'multidex' not in name:
            android_api_level = str(ctx.env['ANDROID_SDK_VERSION_NUMBER'])
            valid_versions = [ ver for ver in versions_list if ver.startswith(android_api_level) ]

        # try to elimiate the alpha, beta and rc versions
        stable_versions = []
        for ver in valid_versions:
            if ('alpha' in ver) or ('beta' in ver) or ('rc' in ver):
                continue
            stable_versions.append(ver)

        if stable_versions:
            return sorted(stable_versions)
        else:
            return sorted(valid_versions)

    # make sure the 3rd Party path is in the env
    if 'THIRD_PARTY' not in ctx.env:
        ctx.env['THIRD_PARTY'] = ctx.tp.calculate_3rd_party_root()

    # first search the local repos from the Android SDK installation
    for local_repo in ANDROID_SDK_LOCAL_REPOS:
        repo_root = string.Template(local_repo).substitute(ctx.env)

        lib_root = os.path.join(repo_root, group_path, name)
        Logs.debug('android_library: Searching %s', lib_root)

        if not os.path.exists(lib_root):
            continue

        if not version or partial_version:

            # filter out all the non-directory, non-numerical entries
            installed_versions = []

            contents = os.listdir(lib_root)
            for entry in contents:
                path = os.path.join(lib_root, entry)
                if os.path.isdir(path) and entry.split('.')[0].isdigit():
                    installed_versions.append(entry)

            valid_versions = _filter_versions(installed_versions)
            if valid_versions:
                Logs.debug('android_library: Valid installed versions of {} found: {}'.format(name, valid_versions))

                highest_useable_version = valid_versions[-1]

                aar_file = '{}-{}.aar'.format(name, highest_useable_version)
                file_path = os.path.join(lib_root, highest_useable_version, aar_file)
                file_url = 'file:{}'.format(file_path)

                if os.path.exists(file_path):
                    return file_url, aar_file

        else:

            aar_file = '{}-{}.aar'.format(name, version)
            file_path = os.path.join(lib_root, version, aar_file)
            file_url = 'file:{}'.format(file_path)

            if os.path.exists(file_path):
                return file_url, aar_file

    # if it's not local, try the main google maven repo
    Logs.debug('android_library: Searching %s', GOOGLE_MAIN_MAVEN_REPO)
    build_google_main_maven_repo_index()

    if group in GOOGLE_MAIN_MAVEN_REPO_INDEX:
        repo_libs = GOOGLE_MAIN_MAVEN_REPO_INDEX[group]
        if name in repo_libs:
            repo_versions = repo_libs[name]

            if not version or partial_version:

                valid_versions = _filter_versions(repo_versions)
                Logs.debug('android_library: Valid repo versions of {} found: {}'.format(name, valid_versions))

                highest_useable_version = valid_versions[-1]

                aar_file = '{}-{}.aar'.format(name, highest_useable_version)
                file_url = '/'.join([ GOOGLE_MAIN_MAVEN_REPO, group_path, name, highest_useable_version, aar_file ])

                return file_url, aar_file

            elif version in repo_versions:

                aar_file = '{}-{}.aar'.format(name, version)
                file_url = '/'.join([ GOOGLE_MAIN_MAVEN_REPO, group_path, name, version, aar_file ])

                return file_url, aar_file

    # finally check the other known google maven repos
    for repo in GOOGLE_BINTRAY_MAVEN_REOPS:
        Logs.debug('android_library: Searching %s', repo)

        global PROTOCOL
        repo_root = '{}://{}'.format(PROTOCOL, repo)

        lib_root = '/'.join([ repo_root, group_path, name ])
        latest_version, all_versions = get_bintray_version_info(lib_root)

        if not (latest_version and all_versions):
            continue

        if not version:
            aar_file = '{}-{}.aar'.format(name, latest_version)
            file_url = '/'.join([ lib_root, latest_version, aar_file ])
            return file_url, aar_file

        elif partial_version:
            valid_versions = _filter_versions(all_versions)
            Logs.debug('android_library: Valid repo versions of {} found: {}'.format(name, valid_versions))

            highest_useable_version = valid_versions[-1]

            aar_file = '{}-{}.aar'.format(name, highest_useable_version)
            file_url = '/'.join([ lib_root, highest_useable_version, aar_file ])

            return file_url, aar_file

        elif version in all_versions:

            aar_file = '{}-{}.aar'.format(name, version)
            file_url = '/'.join([ lib_root, version, aar_file ])

            return file_url, aar_file

    return None, None


###############################################################################
###############################################################################
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

        return result


@conf
def read_aar(conf, name, group = None, version = None, paths = None):
    '''
    Read an Android Archive Resource (local or remote), enabling its use as a local Android library.   Will trigger a rebuild
    if the file changes
    :param conf:    The Context
    :param name:    Name of the library

    :param group:   Maven repository group identifier.  Required for libraries supplied by Google e.g. found in a 'm2repository'
                    directory of the Android SDK.  Cannot be used with the 'paths' parameter.
    :param version: The version number of the Maven hosted library.  If omitted, the version will be auto-detected.  Can only
                    be used with the 'group' parameter.

    :param paths:   Location of the library on disk.  Required for standalone libraries e.g. found outside of a 'm2repository'
                    directory in the Android SDK, 3rd Party libraries, etc.  Cannot be used with the 'group'/'version' parameters.

    def build(bld):
        bld.read_aar(
            name = 'play-services-games',
            group = 'com.google.android.gms',
            version = '11.0.2'
        )

        bld.read_aar(
            name = 'support-v4',
            group = 'com.android.support'
        )

        bld.read_aar(
            name = 'gfxtracer',
            paths = 'C:/Android/android-sdk/extras/android/gapid_3/android'
        )

        bld.AndroidAPK(project_name = 'AwesomeGame', use = [ 'play-services-games', 'support-v4', 'gfxtracer' ])
    '''

    # unable to determine how to locate the library when both types are specified
    if (group or version) and paths:
        conf.Fatal('[ERROR] The "group" (or "version") and "paths" arguments are both specified for call to read_aar.  '
                    'Only one set of arguments can be specified: [ group, version ] OR [ paths ]'
                    'Unable to locate library {}.'.format(name))

    if isinstance(paths, str):
        paths = [ paths ]

    return conf(name = name, features = 'fake_aar', group_id = group, version = version, paths = paths)


@feature('fake_aar')
def process_aar(self):
    '''
    Find the Android library and unpack it so it's resources can be used by other modules
    '''
    def _could_not_find_lib_error():
        raise Errors.WafError('[ERROR] Could not find Android library %r.  Run the command again with "--zones=android_library" included for more information.' % self.name)

    bld = self.bld
    platform = bld.env['PLATFORM']

    # the android studio project generation also requires this to run in order for the
    # aar dependencies to get added to gradle correctly
    if not (bld.is_android_platform(platform) or bld.cmd == 'android_studio'):
        Logs.debug('android_library: Skipping the reading of the aar')
        return

    global PROTOCOL
    if not PROTOCOL:
        PROTOCOL = 'http' if bld.is_option_true('android_maven_force_http') else 'https'

    Utils.def_attrs(
        self,

        manifest = None,
        package = '',

        classpath = [],

        native_libs = [],

        aapt_assets = None,
        aapt_resources = None,
    )

    group = self.group_id
    version = self.version
    search_paths = self.paths

    android_cache = bld.get_android_cache_node()
    aar_cache = android_cache.make_node('aar')
    aar_cache.mkdir()

    Logs.debug('android_library: Processing Android library %s', self.name)
    lib_node = None

    if search_paths:
        aar_filename = '{}.aar'.format(self.name)

        for path in search_paths:
            if not isinstance(path, Node.Node):
                path = bld.root.find_node(path) or self.path.find_node(path)
                if not path:
                    Logs.debug('android_library: Unable to find node for path %s', path)
                    continue

            Logs.debug('android_library: Searching path {}'.format(path.abspath()))
            lib_node = path.find_node(aar_filename)
            if lib_node:
                break
        else:
            _could_not_find_lib_error()

        self.android_studio_name = 'file:{}'.format(lib_node.abspath()).replace('\\', '/')

    else:
        file_url, aar_filename = search_maven_repos(bld, self.name, group, version)

        if not (file_url and aar_filename):
            _could_not_find_lib_error()

        if file_url.startswith('file:'):
            local_path = file_url[5:]

            lib_node = bld.root.find_node(local_path)
            if not lib_node:
                _could_not_find_lib_error()

        else:
            lib_node = aar_cache.find_node(aar_filename)
            if not lib_node:
                lib_node = aar_cache.make_node(aar_filename)
                Logs.debug('android_library: Downloading %s => %s', file_url, lib_node.abspath())

                try:
                    url_opener = urllib.FancyURLopener()
                    url_opener.retrieve(file_url, filename = lib_node.abspath())
                except:
                    bld.Fatal('[ERROR] Failed to download Android library {} from {}.'.format(self.name, file_url))

        if not version:
            version = os.path.splitext(aar_filename)[0].split('-')[-1]

        self.android_studio_name = '{}:{}:{}'.format(group, self.name, version)

    lib_node.sig = Utils.h_file(lib_node.abspath())

    folder_name = os.path.splitext(aar_filename)[0]
    extraction_node = aar_cache.make_node(folder_name)
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

