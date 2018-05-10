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
# Modifications copyright Amazon.com, Inc. or its affiliates.
#
#!/usr/bin/env python
# encoding: utf-8

import argparse
import collections
import io
import json
import mimetypes
import os
import pprint
import re
import stat
import sys
import tarfile

_3RDPARTY_ROOT_IDENTIFIER = '3rdParty.txt'
_ENGINE_ROOT_IDENTIFIER = 'engineroot.txt'
_USER_PREFERENCE_FILE_NAME = 'SetupAssistantUserPreferences.ini'
_USER_PREFERENCE_3RD_PARTY_PATH_KEY_NAME = 'SDKSearchPath3rdParty'
_SETUP_CONFIG_FILE_NAME = 'SetupAssistantConfig.json'
_INTERESTING_ROLES = set([u'compileengine', u'compilegame', u'setuplinux'])
_INTERESTING_HOST_OS = set([u'linux'])
_PROJECT_NAME = 'MultiplayerSample'

# A few force excludes to keep the file size in check.
_EXCLUDED_PATH_PATTERNS = [
    re.compile('3rdparty/Qt/([^/]+?)/clang_64($|/.*)', re.I),
    re.compile('3rdparty/Qt/([^/]+?)/msvc2013_64($|/.*)', re.I),
    re.compile('3rdparty/Qt/([^/]+?)/msvc2015_64($|/.*)', re.I),
    re.compile('3rdparty/Qt/([^/]+?)/patches($|/.*)', re.I),
    re.compile('dev/Code/Sandbox/SDKs($|/.*)', re.I),
    re.compile('dev/Code/SDKs($|/.*)', re.I),
    re.compile('dev/Code/Tools/SDKs($|/.*)', re.I),
    re.compile('dev/Gems/PBSreferenceMaterials($|/.*)', re.I),
    re.compile('dev/Gems/LyShineExamples($|/.*)', re.I),
    re.compile('dev/Gems/Clouds($|/.*)', re.I),
    re.compile('dev/Gems/Boids($|/.*)', re.I),
    re.compile('dev/Gems/EMotionFX($|/.*)', re.I),
    re.compile('dev/Tools/3dsmax($|/.*)', re.I),
    re.compile('dev/Tools/AWSNativeSDK($|/.*)', re.I),
    re.compile('dev/Tools/crashpad($|/.*)', re.I),
    re.compile('dev/Tools/CryMaxTools($|/.*)', re.I),
    re.compile('dev/Tools/CrySCompileServer($|/.*)', re.I),
    re.compile('dev/Tools/GFxExport($|/.*)', re.I),
    re.compile('dev/Tools/lmbrsetup/win($|/.*)', re.I),
    re.compile('dev/Tools/LuaRemoteDebugger($|/.*)', re.I),
    re.compile('dev/Tools/maxscript($|/.*)', re.I),
    re.compile('dev/Tools/maya($|/.*)', re.I),
    re.compile('dev/Tools/melscript($|/.*)', re.I),
    re.compile('dev/Tools/photoshop($|/.*)', re.I),
    re.compile('dev/Tools/Python/2.7.12/windows($|/.*)', re.I),
    re.compile('dev/Tools/Python/2.7.13($|/.*)', re.I),
    re.compile('dev/Tools/Redistributables($|/.*)', re.I),
    re.compile('dev/Tools/RemoteConsole($|/.*)', re.I),
    re.compile('(3rdparty|dev)/.+?/(lib|bin)/(ios|steamos|android|appletv|mac|osx|vc120|vc140|x86|x64|win|msvc)($|/.*)', re.I),
]

_EXCLUDED_EXTENSIONS = set([
    '.0svn',
    '.app', '.appletv',
    '.bat',
    '.dll', '.dmg', '.dsp', '.dsw',
    '.exe',
    '.lib',
    '.ma', '.max', '.mingw', '.modulemap',
    '.pbxproj', '.pdb', '.psd', '.pyc',
    '.sdf',
    '.tga',
    '.vcproj', '.vcxproj' '.vspscc', '.vssscc',
    '.xbm', '.xcscheme', '.xcsettings', '.xcworkspacedata', '.xib'
])

_PRINTABLE_CHARS = bytearray({7,8,9,10,12,13,27} | set(range(0x20, 0x100)) - {0x7f})

_NON_EXECUTABLE_EXTENSIONS = set(['.0',
    '.1', '.2', '.20', '.21', '.22', '.23', '.3', '.5', '.56', '.6', '.7', '.7-32', '.7-config', '.8bf',
    '.a', '.abc', '.ac', '.actions', '.aif', '.aifc', '.aiff', '.am', '.animsettings', '.any', '.app', '.applescript',
    '.appletv', '.appxmanifest', '.args', '.assetinfo', '.atn', '.au', '.awk',

    '.bai', '.bat', '.bin', '.bmp', '.bnk', '.bpf', '.bpg', '.bpr',

    '.c', '.cab', '.caf', '.cal', '.cbc', '.cc', '.cd', '.cdf', '.cfg', '.cfi', '.cfx', '.cgf', '.chm', '.chr',
    '.chrparams', '.cmake', '.cmakein', '.cmd', '.cnf', '.com', '.conf', '.config', '.cpp', '.creator', '.crl',
    '.cross', '.cry', '.cs', '.csc', '.csproj', '.css', '.ctc', '.ctypes', '.cur', '.cxx',

    '.dat', '.datasource', '.db', '.dds', '.dectest', '.def', '.der', '.dll', '.dlu', '.doc', '.dox', '.dsp', '.dsw',
    '.dtd', '.dylib',

    '.egg-info', '.enc', '.ent', '.eps', '.example', '.exe', '.exp', '.exportsettings', '.ext',

    '.fbx', '.file_list', '.files', '.filters', '.font', '.fontfamily', '.fs', '.fsc', '.fx',

    '.gif', '.git', '.gitignore', '.glsl', '.gnu', '.guess', '.gz', '.h', '.hlsl', '.hpp',
    '.htc', '.htm', '.html', '.hxx',

    '.icc', '.icns', '.ico', '.idl', '.ignore', '.ignore_when_copying', '.import', '.in', '.includes', '.inf',
    '.info', '.ini', '.inl', '.inputbindings', '.install', '.ios', '.ipp', '.iss',

    '.java', '.jpg', '.js', '.json',

    '.key', '.keystore',

    '.la', '.lastbuildstate', '.layout', '.lib', '.local', '.lof', '.log', '.lua', '.lut', '.lyr',

    '.m', '.m4', '.ma', '.mac', '.mak', '.man', '.manifest', '.manpages', '.map', '.markdown', '.mask', '.max', '.md',
    '.mel', '.mf', '.minimal', '.mk', '.mm', '.mmp', '.mms', '.mpw', '.ms', '.msg', '.msvc', '.mtl',

    '.natjmc', '.natstepfilter', '.natvis', '.njsproj', '.normal',

    '.o', '.obj', '.odt', '.options', '.os4', '.otf', '.out',

    '.p4ignore', '.pak', '.pandora', '.patch', '.pbm', '.pbxproj', '.pc', '.pck', '.pdb', '.pdf', '.pem', '.pgm',
    '.pickle', '.pl', '.plist', '.png', '.ppm', '.prf', '.pri', '.prj', '.prl', '.pro', '.properties', '.props',
    '.psd', '.psp', '.pssl', '.pump', '.py', '.pyc', '.pyd', '.pyo', '.pyproj', '.pys', '.pyw',

    '.qm', '.qml', '.qmltypes', '.qph', '.qrc', '.qss',

    '.r', '.ras', '.raw', '.rc', '.rc2', '.readme', '.rej', '.rels', '.resx', '.rgs', '.rsrc', '.rst', '.rtf',

    '.sample', '.sbsar', '.scss', '.sdf', '.sed', '.settings', '.sfo', '.sgi', '.sgml', '.shared', '.sic', '.skin',
    '.slice', '.sln', '.snk', '.so', '.spec', '.sprite', '.storyboard', '.strip', '.sub', '.sunwcch', '.svg', '.svn',

    '.tar', '.targets', '.tcl', '.terms', '.tex', '.tga', '.thread_config', '.tif', '.tiff', '.tlb', '.tlog', '.tm',
    '.tpl', '.trp', '.ts', '.ttf', '.txt',

    '.ui', '.uicanvas', '.uiprefab', '.uncrustify', '.unix', '.user', '.uue',

    '.vbs', '.vcproj', '.vcxproj', '.ver', '.vms', '.vspscc', '.vssscc',

    '.waf_files', '.waf_files_backup', '.wav', '.whl', '.wiz', '.wsdl',

    '.xbm', '.xcscheme', '.xcsettings', '.xcworkspacedata', '.xib', '.xml', '.xpm', '.yml',

    '.zip'
])

_EXECUTABLE_EXTENSIONS = set([
    '.pl', '.sh'
])


def _conform_path(path):
    '''
    Do the necessary to standardize input path and return the resultant path.
    '''
    return path.replace('\\', '/')

def _get_datetime_as_tar_filename():
    import time
    return time.strftime('%Y-%m-%d_%H-%M-%S.tar', time.localtime())

def _get_dev_sources_root(cwd):
    '''
    Walk up from the current directory in search for the file that marks the engine's root.
    Returns the containing directory.
    '''
    directory = ''
    parent_directory = cwd

    while parent_directory and (directory != parent_directory):
        directory = parent_directory
        filepath = os.path.join(directory, _ENGINE_ROOT_IDENTIFIER)
        if os.path.isfile(filepath):
            return _conform_path(directory)

        parent_directory = os.path.dirname(parent_directory)

    raise RuntimeError('Failed to locate engine root directory. Missing file: ' + _ENGINE_ROOT_IDENTIFIER)

def _get_dev_destination_root():
    '''
    Returns the relative path to dev folder within the archive
    '''
    return _PROJECT_NAME + '/dev'

def _get_3rdparty_sources_root(user_preference_file_path):
    '''
    Parse User configuration file to locate the path to 3rd party sources
    '''
    if not os.path.isfile(user_preference_file_path):
        raise RuntimeError('Failed to locate user configuration file. Missing file: ' + _USER_PREFERENCE_FILE_NAME)

    with open(user_preference_file_path, 'rt') as istrm:
        for line in istrm:
            parts = line.split('=')
            if len(parts) == 2 and parts[0].strip() == _USER_PREFERENCE_3RD_PARTY_PATH_KEY_NAME:
                return _conform_path(os.path.abspath(parts[1].strip()))

    raise RuntimeError('3rd party path missing in user configuration file: ' + user_preference_file_path)

def _get_3rdparty_destination_root():
    '''
    Returns the relative path for 3rd party dependencies within the archive
    '''
    return _PROJECT_NAME + '/3rdParty'

def _build_3rdparty_sources_map(config_file_path, source_root, destination_root):
    '''
    Return a map of absolute path to relative paths of all 3rd party dependencies
    '''
    def _is_interesting(sdk_node, symlink_node):
        '''
        Returns true if the input json node from config file needs to be considered further
        to collect sources for processing.
        '''
        roles = None
        hosts = None
        compilers = None
        optional = None
        for node in [symlink_node, sdk_node]:
            roles = roles or node.get('roles')
            hosts = hosts or node.get('hostOS')
            compilers = compilers or node.get('compilers')
            optional = optional or ([node['optional']] if 'optional' in node else None)

        role_check = (not roles) or (len(set.intersection(_INTERESTING_ROLES, set(roles))) > 0)
        host_check = (not hosts) or ('linux' in hosts)
        compiler_check = (not compilers) or ('gcc' in compilers) or ('clang' in compilers)
        optional_check = optional and (optional == [1])
        return role_check and host_check and compiler_check and (not optional_check)

    with open(config_file_path, 'rt') as istrm:
        config = json.load(istrm)
        istrm.close()

    dependencies = [symlink for sdk in config['SDKs']
                    for symlink in sdk['symlinks'] if _is_interesting(sdk, symlink)]

    sources = {
        _conform_path(os.path.join(source_root, dependency['source']).encode('utf-8')) :
            _conform_path(os.path.join(destination_root, dependency['source']).encode('utf-8'))
                for dependency in dependencies
    }

    # Include the 3rd party folder identifier
    identifier_filepath = _conform_path(os.path.join(source_root, _3RDPARTY_ROOT_IDENTIFIER))
    sources[identifier_filepath] = destination_root + '/' + _3RDPARTY_ROOT_IDENTIFIER
    return sources

def _build_dev_sources_map(source_root, destination_root):
    '''
    Return a map of absolute path to relative paths of all engine/dev dependencies
    '''
    relative_file_paths = set([
        '_WAF_',
        'Cache/%s/pc' % _PROJECT_NAME,
        'Code',
        'Engine',
        'Gems',
        _PROJECT_NAME,
        '%s_pc_Paks_Dedicated' % _PROJECT_NAME,
        'ProjectTemplates',
        'Tools',
        '.p4ignore',
        'AssetProcessorPlatformConfig.ini',
        'bootstrap.cfg',
        '%s_CreateGameLiftPackage.sh' % _PROJECT_NAME,
        'editor.cfg',
        'engine.json',
        'engineroot.txt',
        'lmbr_waf.sh',
        'lmbr_test.sh',
        'lmbr_test_blacklist.txt',
        'LyzardConfig.xml',
        'SetupAssistantConfig.json',
        'SetupAssistantUserPreferences.ini',
        'shadercachegen.cfg',
        'UserSettings.xml',
        'waf_branch_spec.py',
        'wscript'
    ])

    return {
        _conform_path(os.path.join(source_root, rel_path).encode('utf-8')) :
            _conform_path(os.path.join(destination_root, rel_path).encode('utf-8'))
                for rel_path in relative_file_paths
    }


def _is_executable(name, source_root_dev, source_root_3rdparty):
    if name.startswith('3rdparty'):
        path = os.path.join(source_root_3rdparty, name[len('3rdparty') + 1:])
    else:
        path = os.path.join(source_root_dev, name[len('dev') + 1:])

    # Ref: https://stackoverflow.com/questions/898669/how-can-i-detect-if-a-file-is-binary-non-text-in-python
    return bool(open(path, 'rb').read(1024).translate(None, _PRINTABLE_CHARS))

_file_extensions = set()
_file_mimetypes = set()
def _create_tarfile(sources_map, source_root_dev, source_root_3rdparty, output_filepath):
    '''
    Iterate input sources map and add each of the source to create a tar ball
    '''
    def _filter(tarinfo):
        '''
        Helper function that returns True/False depending on whether the specific
        file should be included in the final tar ball or not.
        '''
        name = tarinfo.name[len(_PROJECT_NAME) + 1:].lower()

        if tarinfo.isfile():
            # Exclude the file if its in exluded extensions
            extension = os.path.splitext(name)[1]
            if extension:
                _file_extensions.add(extension)
                if extension in _EXCLUDED_EXTENSIONS:
                    return None

            # Try to deduce file permissions based on mimetype
            type, _ = mimetypes.guess_type(name)
            if type:
                _file_mimetypes.add(type)
                type = type.split('/')[0]

            if (extension in _NON_EXECUTABLE_EXTENSIONS) or (type in set(['text', 'image', 'audio'])):
                pass # Do nothing!
            elif (extension in _EXECUTABLE_EXTENSIONS) or _is_executable(name, source_root_dev, source_root_3rdparty):
                tarinfo.mode = tarinfo.mode | stat.S_IXUSR
        else:
            # Directories need to be marked as executables
            tarinfo.mode = tarinfo.mode | stat.S_IXUSR

        for pattern in _EXCLUDED_PATH_PATTERNS:
            if pattern.match(name):
                return None

        tarinfo.mode = tarinfo.mode & ~(stat.S_IRWXG | stat.S_IRWXO)    # Group and Others have no read/write/execute permission
        tarinfo.mode = tarinfo.mode | stat.S_IRUSR | stat.S_IWUSR       # Owner has read/write permission
        return tarinfo

    # Make sure the directory exists
    directory = os.path.dirname(output_filepath)
    if not os.path.exists(directory):
        os.makedirs(directory)

    print('Creating tar file at ' + output_filepath)
    with tarfile.open(output_filepath, 'w') as ostrm:
        for src, dst in sources_map.iteritems():
            print('Adding {0} => {1} ...'.format(src, dst))
            ostrm.add(src, dst, filter=_filter)

        ostrm.close()

def _compress_tarfile(input_filepath):
    output_filepath = input_filepath + '.gz'
    with tarfile.open(output_filepath, 'w:gz') as ostrm:
        ostrm.add(input_filepath)
        ostrm.close()

    return output_filepath

def main():
    parser = argparse.ArgumentParser('Script to create an archive of sources to build dedicated linux server')
    parser.add_argument('-c', '--cwd', type=str, help='Current working directory')
    parser.add_argument('-p', '--3rdparty', dest='third_party_sources_root', type=str,
                        help='Path to source 3rd party dependencies')
    parser.add_argument('-x', '--compress', action='store_true', help='Compress the final archive?')
    parser.add_argument('-o', '--output', type=str, help='Path to output file to generate')
    parser.add_argument('-ext', '--print_extensions', action='store_true', help='Print a list of all extensions found')
    parser.add_argument('-types', '--print_filetypes', action='store_true', help='Print a list of all filetypes found')
    args = parser.parse_args()

    cwd = _conform_path(args.cwd or os.path.dirname(os.path.abspath(__file__)))

    # Build dev sources map
    source_root_dev = _get_dev_sources_root(cwd)
    destination_root_dev = _get_dev_destination_root()
    sources_map_dev = _build_dev_sources_map(source_root_dev, destination_root_dev)

    # Build 3rd party sources map
    setup_config_file_path = os.path.join(source_root_dev, _SETUP_CONFIG_FILE_NAME)
    user_preference_file_path = os.path.join(source_root_dev, _USER_PREFERENCE_FILE_NAME)
    source_root_3rdparty = args.third_party_sources_root or _get_3rdparty_sources_root(user_preference_file_path)
    destination_root_3rdparty = _get_3rdparty_destination_root()

    # Make sure the input/computed 3rd party path is valid
    if not os.path.isfile(os.path.join(source_root_3rdparty, _3RDPARTY_ROOT_IDENTIFIER)):
        raise RuntimeError('3rd party identifier file not found: ' + filepath)

    sources_map_3rdparty = _build_3rdparty_sources_map(
        setup_config_file_path, source_root_3rdparty, destination_root_3rdparty)

    # Combine the sources map and print it out for records
    sources_map = collections.OrderedDict(sources_map_3rdparty, **sources_map_dev)
    pprint.pprint({ key: value for key, value in sources_map.iteritems() })
    print('\n')

    if not args.output:
        args.output = _conform_path(os.path.join(
            source_root_dev, 'BinTemp', 'unix_archives', _get_datetime_as_tar_filename()))

    # Finally create the archive
    _create_tarfile(sources_map, source_root_dev, source_root_3rdparty, args.output)
    print('Uncompressed archive successfully generated at ' + args.output)
    print('\n')

    if args.print_extensions:
        print('List of all the extension(s) that were found:')
        pprint.pprint(_file_extensions)

        unknown_extensions = _file_extensions.difference(_NON_EXECUTABLE_EXTENSIONS.union(_EXECUTABLE_EXTENSIONS))
        if unknown_extensions:
            print('\n')
            print('List of unknown extensions found')
            pprint.pprint(unknown_extensions)
        print('\n')

    if args.print_filetypes:
        print('List of all the mimetype(s) that were found')
        pprint.pprint(_file_mimetypes)
        print('\n')

    if args.compress:
        compressed_output_path = _compress_tarfile(args.output)
        print('Compressed archive successfully generated at ' + compressed_output_path)
        print('\n')

    print('Archive(s) successfully generated!!')

if __name__ == "__main__":
    sys.exit(main())
