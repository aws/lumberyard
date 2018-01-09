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

from waflib.TaskGen import feature, before_method, after_method
from waflib.Configure import conf, Logs
from waflib.Tools.ccroot import lib_patterns, SYSTEM_LIB_PATHS
from waflib import Node, Utils, Errors
from waflib.Build import BuildContext
from waf_branch_spec import PLATFORMS
from utils import fast_copy2, should_overwrite_file
import stat
import os


def is_win_x64_platform(ctx):
    platform = ctx.env['PLATFORM'].lower()
    return ('win_x64' in platform) and (platform in PLATFORMS['win32'])
    
def is_darwin_x64_platform(ctx):
    platform = ctx.env['PLATFORM'].lower()
    return ('darwin_x64' in platform) and (platform in PLATFORMS['darwin'])


def use_windows_dll_semantics(ctx):
    if is_win_x64_platform(ctx):
        return True
    platform = ctx.env['PLATFORM'].lower()
    return False


def get_static_suffix():
    return "_static"


def get_shared_suffix():
    return "_shared"


def should_link_aws_native_sdk_statically(bld):
    platform = bld.env['PLATFORM']
    configuration = bld.env['CONFIGURATION']

    if ((platform != 'project_generator' and bld.is_variant_monolithic(platform, configuration)) or
        any(substring in platform for substring in [
            'darwin',
            'ios',
            'appletv',
            'linux',
            'android',
        ])):
        return True
    return False


def get_dynamic_lib_extension(bld):
    platform = bld.env['PLATFORM']
    if any(substring in platform for substring in ['darwin', 'ios', 'appletv']):
        return '.dylib'
    elif any(substring in platform for substring in ['linux', 'android']):
        return '.so'
    return '.dll'


def get_platform_lib_prefix(bld):
    platform = bld.env['PLATFORM']
    if any(substring in platform for substring in [
        'darwin',
    'ios',
    'appletv',
    'linux',
    'android',
    ]):
        return 'lib'
    return ''


def aws_native_sdk_platforms(bld):
    return [
        'win',
        'darwin',
        'ios',
        'appletv',
        'linux',
        'android_armv7_clang',
        'android_armv8_clang',
        'project_generator']


def should_project_include_aws_native_sdk(bld):
    platform = bld.env['PLATFORM']
    if 'project_generator' in platform:
        return True  # Build
    return does_platform_support_aws_native_sdk(bld)


def does_platform_support_aws_native_sdk(bld):
    platform = bld.env['PLATFORM']
    if any(substring in platform for substring in aws_native_sdk_platforms(bld)):
        return True
    return False


@conf
def add_aws_native_sdk_platform_defines(self, define_list):
    if should_project_include_aws_native_sdk(self):
        define_list.append('PLATFORM_SUPPORTS_AWS_NATIVE_SDK')
        if use_windows_dll_semantics(self):
            define_list.append('USE_WINDOWS_DLL_SEMANTICS')


@conf
def make_static_library_task_name(self, libName):
    return libName + get_static_suffix()


@conf
def make_shared_library_task_name(self, libName):
    return libName + get_shared_suffix()


@conf
def make_shared_library_task_list(self, libraryList):
    return [self.make_shared_library_task_name(l) for l in libraryList]


@conf
def make_static_library_task_list(self, libraryList):
    return [self.make_static_library_task_name(l) for l in libraryList]


@conf
def make_aws_library_task_list(self, libraryList):
    # Special case for LyMetrics/LyIdentity since we don't ship the source code, instead we use the pre-built libs
    extra_lib = []
    if 'LyMetricsProducer' in libraryList:
        if 'LyMetricsShared' not in libraryList:
            libraryList.append('LyMetricsShared')
        extra_lib.append('AWSNativeSDKInit')
    if 'LyMetricsShared' in libraryList and 'LyIdentity' not in libraryList:
        libraryList.append('LyIdentity')

    if self.cmd == 'generate_module_def_files':
        # Special case, if this command is being run, we always want to consider both
        # shared and static when generated the module_def file
        return self.make_static_library_task_list(libraryList) + self.make_shared_library_task_list(libraryList) + extra_lib

    shouldLinkStatically = isinstance(self, BuildContext) and should_link_aws_native_sdk_statically(self)
    if shouldLinkStatically:
        return self.make_static_library_task_list(libraryList) + extra_lib
    else:
        return self.make_shared_library_task_list(libraryList) + extra_lib


def convert_dual_task_name_to_lib_name(taskName):
    shared_index = taskName.find(get_shared_suffix())
    if shared_index >= 0:
        return taskName[0:shared_index]
    else:
        return taskName[0:taskName.find(get_static_suffix())]


@conf
def read_dual_library_shared(self, name, paths=[], export_includes=[], export_defines=[]):
    """
    Read a system shared library, enabling its use as a local library.   Performs name-mangling on the task name to ensure uniqueness with
    a static version of the same library.  Will trigger a rebuild if the file changes.

    """

    return self(name=self.make_shared_library_task_name(name), features='fake_dual_lib', lib_paths=paths,
                lib_type='shlib', export_includes=export_includes, export_defines=export_defines)


@conf
def read_dual_library_static(self, name, paths=[], export_includes=[], export_defines=[]):
    """
    Read a system static library, enabling a use as a local library. Performs name-mangling on the task name to ensure uniqueness with
    a shared version of the same library.  Will trigger a rebuild if the file changes.
    """
    return self(name=self.make_static_library_task_name(name), features='fake_dual_lib', lib_paths=paths,
                lib_type='stlib', export_includes=export_includes, export_defines=export_defines)


@feature('fake_dual_lib')
def process_dual_lib(self):
    """
    Find the location of a foreign library. Used by :py:class:`lmbrwaflib.lumberyard_sdks.read_dual_library_shared` and
    :py:class:`lmbrwaflib.lumberyard_sdks.read_dual_library_static`.
    """
    node = None
    library_name = convert_dual_task_name_to_lib_name(self.name)

    names = [x % library_name for x in lib_patterns[self.lib_type]]
    for x in self.lib_paths + [self.path] + SYSTEM_LIB_PATHS:
        if not isinstance(x, Node.Node):
            x = self.bld.root.find_node(x) or self.path.find_node(x)
            if not x:
                continue

        for y in names:
            node = x.find_node(y)
            if node:
                node.sig = Utils.h_file(node.abspath())
                break
        else:
            continue
        break
    else:
        raise Errors.WafError('could not find library %r' % library_name)
    self.link_task = self.create_task('fake_%s' % self.lib_type, [], [node])
    if not getattr(self, 'target', None):
        self.target = library_name
    if not getattr(self, 'output_file_name', None):
        self.output_file_name = library_name


@conf
def install_internal_sdk_headers(bld, build_task_name, sdk_name, header_list):
    if isinstance(bld, BuildContext):
        try:
            task_gen = bld.get_tgen_by_name(bld.make_shared_library_task_name(build_task_name))
            if task_gen != None:
                for header in header_list:
                    task_gen.create_task('copy_outputs',
                                         task_gen.path.make_node(build_task_name + '/include/' + header),
                                         bld.engine_node.make_node('Tools/InternalSDKs/' + sdk_name + '/include/' + header))
        except:
            return


def _get_build_dir(self, forceReleaseDir=False):
    build_dir = 'x64'  # default to x64, unless we have a better case

    platform = self.env['PLATFORM'].lower()
    # Return early for these platforms as they don't follow the same directory
    # structure or naming convention as window platforms
    if 'ios' in platform:
        build_dir = 'ios'
        return build_dir
    elif 'appletv' in platform:
        build_dir = 'appletv'
        return build_dir
    elif 'mac' in platform:
        build_dir = 'OSX'
        return build_dir
    elif 'linux' in platform:
        build_dir = 'linux'
        return build_dir
    elif 'android' in platform:
        build_dir = 'android'
        return build_dir

    build_dir += '/'

    config = self.env['CONFIGURATION'].lower()
    if forceReleaseDir is True:
        build_dir += 'Release'
    else:

        if 'debug' in config:
            build_dir += 'Debug'
        elif 'profile' in config:
            build_dir += 'Release'
        else:
            build_dir += 'Release'

    return build_dir


@conf
def BuildPlatformLibraryDirectory(bld, forceStaticLinking):
    config = bld.env['CONFIGURATION']
    platform = bld.env['PLATFORM']

    if 'debug' in config:
        configDir = 'Debug'
    else:
        configDir = 'Release'

    if forceStaticLinking:
        libDir = 'lib'
    else:
        libDir = 'bin'

    if 'ios' in platform:
        platformDir = 'ios'
    elif 'appletv' in platform:
        platformDir = 'appletv'
    elif 'darwin' in platform:
        platformDir = 'mac'
    elif 'linux' in platform:
        platformDir = 'linux/intel64'
    elif 'android' in platform:
        platformDir = 'android'
    else:
        # TODO: add support for other platforms as versions become available
        platformDir = 'windows/intel64'
        compilerDir = None

        if bld.env['MSVC_VERSION'] == 14:
            compilerDir = 'vs2015'
        elif bld.env['MSVC_VERSION'] == 12:
            compilerDir = 'vs2013'

        if compilerDir != None:
            platformDir += '/{}'.format(compilerDir)

    return '{}/{}/{}'.format(libDir, platformDir, configDir)


@feature('AWSNativeSDK')
@after_method('apply_monolithic_pch_objs')
def link_aws_sdk_core_after(self):
    # AWSNativeSDK has a requirement that the aws-cpp-sdk-core library be linked after other aws libraries
    # The use system make this difficult as it adds dependent libraries in a first-seen order, and this results
    # in the wrong link order for monolithic builds.
    # This function removes the library from the use system after includes are applied but before the libs are
    # calculated, and instead adds the library to the libs, which are added after the use libraries, fixing the link order
    platform = self.env['PLATFORM']
    if 'linux' in platform and 'aws-cpp-sdk-core_static' in self.use:
        self.use.remove('aws-cpp-sdk-core_static')
        if 'aws-cpp-sdk-core' not in self.env['LIB']:
            self.env['LIB'] += ['aws-cpp-sdk-core']


@feature('ExternalLyIdentity')
@before_method('apply_incpaths')
def copy_external_ly_identity(self):
    pass

@feature('ExternalLyMetrics')
@before_method('apply_incpaths')
def copy_external_ly_metrics(self):
    pass

def get_python_home_lib_and_dll(ctx, platform):
    """
    Get the following (internal) python paths based on the platform
    1.  Home path
    2.  Includes path
    3.  Lib path
    4.  Path to the shared object (dll)

    :param ctx:         The build context
    :param platform:    The platform to key the search
    :return:    see the description
    """

    if platform in ['win_x64_vs2013', 'win_x64_vs2015', 'win_x64_test', 'project_generator']:
        python_version = '2.7.12'
        python_variant = 'windows'
        python_includes = 'include'
        python_libs = 'libs'
        python_dll_name = 'python27.dll'
        python_home = os.path.join(ctx.engine_path, 'Tools', 'Python', python_version, python_variant)
    elif platform == 'linux_x64':
        python_version = '2.7.12'
        python_variant = 'linux_x64'
        python_includes = 'include/python2.7'
        python_libs = 'lib'
        python_dll_name = 'lib/libpython2.7.so'
        python_home = os.path.join(ctx.engine_path, 'Tools', 'Python', python_version, python_variant)
    elif platform == 'darwin_x64':
        python_version = '2.7.13'
        python_variant = 'mac'
        python_includes = 'Headers'
        python_libs = 'Versions/Current/lib'
        python_dll_name = 'Versions/2.7/lib/libpython2.7.dylib'
        python_home = os.path.join(ctx.engine_path, 'Tools', 'Python', python_version, python_variant, 'Python.framework')
    else:
        raise Errors.WafError('Python dll not supported for platform {}'.format(platform))


    python_includes_dir = os.path.join(python_home, python_includes)
    python_libs_dir = os.path.join(python_home, python_libs)
    python_dll = os.path.join(python_home, python_dll_name)
    return python_home, python_includes_dir, python_libs_dir, python_dll


def copy_local_python_to_target(task, source_python_dll_path):
    """
    Perform a quick simple copy of python.dll from a local python folder

    :param task:                    The current running task
    :param source_python_dll_path:  The source python path
    """
    current_platform = task.env['PLATFORM'].lower()
    current_configuration = task.env['CONFIGURATION'].lower()

    # Determine the target folder(s)
    output_sub_folder = getattr(task, 'output_sub_folder', None)

    # If we have a custom output folder, then make a list of nodes from it
    target_folders = getattr(task, 'output_folder', [])
    if len(target_folders) > 0:
        target_folders = [task.bld.path.make_node(node) if isinstance(node, str) else node for node in target_folders]
    else:
        target_folders = task.bld.get_output_folders(current_platform, current_configuration)

    python_name = os.path.basename(source_python_dll_path)

    # Copy to each output folder target node
    for target_node in target_folders:

        python_dll_target_path = os.path.join(target_node.abspath(), python_name)
        if output_sub_folder:
            python_dll_target_path = os.path.join(target_node.make_node(output_sub_folder).abspath(), python_name)
        if should_overwrite_file(source_python_dll_path, python_dll_target_path):
            try:
                # In case the file is readonly, we'll remove the existing file first
                if os.path.exists(python_dll_target_path):
                    os.chmod(python_dll_target_path, stat.S_IWRITE)
                fast_copy2(source_python_dll_path, python_dll_target_path)
            except Exception as e:
                Logs.warn('[WARN] Unable to copy {} to destination {} ({}).  '
                          'Check the file permissions or any process that may be locking it.'
                          .format(python_dll_target_path, python_dll_target_path, e.message))


@feature('EmbeddedPython')
@before_method('apply_incpaths')
def enable_embedded_python(self):
    # Only win_x64 builds support embedding Python.
    #
    # We assume 'project_generator' (the result of doing a lmbr_waf configure) is
    # also for a win_x64 build so that the include directory is configured property
    # for Visual Studio.

    platform = self.env['PLATFORM'].lower()
    config = self.env['CONFIGURATION'].lower()

    if platform in ['win_x64_vs2013', 'win_x64_vs2015', 'win_x64_test', 'linux_x64', 'project_generator']:

        # Set the USE_DEBUG_PYTHON environment variable to the location of
        # a debug build of python if you want to use one for debug builds.
        #
        # This does NOT work with the boost python helpers, which are used
        # by the Editor, since the boost headers always undef _DEBUG before
        # including the python headers.
        #
        # Any code that includes python headers directly should also undef
        # _DEBUG before including those headers except for when USE_DEBUG_PYTHON
        # is defined.

        if 'debug' in config and 'USE_DEBUG_PYTHON' in os.environ:

            python_home = os.environ['USE_DEBUG_PYTHON']
            python_dll = '{}/python27_d.dll'.format(python_home)

            self.env['DEFINES'] += ['USE_DEBUG_PYTHON']

        else:
            python_home, python_include_dir, python_libs_dir, python_dll = get_python_home_lib_and_dll(self.bld,
                                                                                                       platform)

        self.includes += [python_include_dir]
        self.env['LIBPATH'] += [python_libs_dir]

        # Save off the python home for use from within code (BoostPythonHelpers.cpp).  This allows us to control exactly which version of
        # python the editor uses.
        # Also, standardize on forward-slash through the entire path.  We specifically don't use backslashes to avoid an interaction with compiler
        # response-file generation in msvcdeps.py and mscv_helper.py that "fixes up" all the compiler flags, in part by replacing backslashes
        # with double-backslashes.  If we tried to replace backslashes with double-backslashes here to make it a valid C++ string, it would
        # get double-fixed-up in the case that a response file gets used (long directory path names).
        # Side note - BoostPythonHelpers.cpp, which uses this define, apparently goes through and replaces forward-slashes with backslashes anyways.
        if python_home.startswith(self.bld.engine_path):
            python_home_define = '@root@{}'.format(python_home[len(self.bld.engine_path):])
        else:
            python_home_define = python_home

        self.env['DEFINES'] += ['DEFAULT_LY_PYTHONHOME="{}"'.format(python_home_define.replace('\\', '/'))]
        if 'LIBPATH_BOOSTPYTHON' in self.env:
            self.env['LIBPATH'] += self.env['LIBPATH_BOOSTPYTHON']
        elif self.env['PLATFORM'] != 'project_generator':
            Logs.warn(
                '[WARN] Required 3rd party boostpython not detected.  This may cause a link error in project {}.'.format(
                    self.name))

    if platform in ['darwin_x64']:
        _, python_include_dir, python_libs_dir, _ = get_python_home_lib_and_dll(self.bld, platform)

        # TODO: figure out what needs to be set for OSX builds.
        self.includes += [python_include_dir]
        self.env['LIBPATH'] += [python_libs_dir]


@feature('ApplyEmbeddedPythonDependency', 'EmbeddedPython')
@before_method('apply_incpaths')
def apply_embedded_python_dependency(self):
    """
    Ideally we would load python27.dll from the python home directory. The best way
    to do that may be to delay load python27.dll and use SetDllDirectory to insert
    the python home directory into the DLL search path. However that doesn't work
    because the boost python helpers import a data symbol.
    :param self:    The current task
    """
    current_platform = self.bld.env['PLATFORM']

    if current_platform in ['win_x64_vs2013', 'win_x64_vs2015', 'win_x64_test', 'linux_x64']:
        # Only supported for win_x64 and linux
        _, _, _, python_dll = get_python_home_lib_and_dll(self.bld, current_platform)
        copy_local_python_to_target(self, python_dll)


@feature('internal_telemetry')
@before_method('apply_incpaths')
def stub_internal_telemetry(self):
    pass


def _apply_to_attribute(self, output_attr_name, bin_relative_output_folder):

    if self.bld.is_engine_local():
        # Only consider copying to the tools folder if we are building from the engine root

        # Go up one level to apply the relative path to the eventual output folder
        output_path = os.path.join('..', bin_relative_output_folder)

        current_attr = getattr(self,output_attr_name,None)
        if current_attr:
            if isinstance(current_attr,list):
                current_attr.append(output_path)
            else:
                current_attr = [current_attr, output_path]
        else:
            current_attr = output_path
        setattr(self, output_attr_name, current_attr)


@feature('copy_to_lmbrsetup')
@before_method('set_link_outputs')
def copy_to_tools_lmbr_setup(self):
    _apply_to_attribute(self, 'output_sub_folder_copy', self.bld.get_lmbr_setup_tools_output_folder())

