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
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#

from waflib import Logs
from waflib.Configure import conf
from waflib.TaskGen import feature, before_method, after_method
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waflib.Errors import WafError
import subprocess
import re

# the default file permissions after copies are made.  511 => 'chmod 777 <file>'
FILE_PERMISSIONS = 511

@conf
def load_darwin_common_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin configurations
    """
    v = conf.env
    
    # Setup common defines for darwin
    v['DEFINES'] += [ 'APPLE', 'MAC', '__APPLE__', 'DARWIN' ]
    
    # Set Minimum mac os version to 10.10
    v['CFLAGS'] += [ '-mmacosx-version-min=10.10', '-msse4.2' ]
    v['CXXFLAGS'] += [ '-mmacosx-version-min=10.10', '-msse4.2' ]
    v['LINKFLAGS'] += [ '-mmacosx-version-min=10.10', '-Wl,-undefined,error', '-headerpad_max_install_names']
    
    # For now, only support 64 bit MacOs Applications
    v['ARCH'] = ['x86_64']
    
    # Pattern to transform outputs
    v['cprogram_PATTERN']   = '%s'
    v['cxxprogram_PATTERN'] = '%s'
    v['cshlib_PATTERN']     = 'lib%s.dylib'
    v['cxxshlib_PATTERN']   = 'lib%s.dylib'
    v['cstlib_PATTERN']      = 'lib%s.a'
    v['cxxstlib_PATTERN']    = 'lib%s.a'
    v['macbundle_PATTERN']    = 'lib%s.dylib'
    
    # Specify how to translate some common operations for a specific compiler   
    v['FRAMEWORK_ST']       = ['-framework']
    v['FRAMEWORKPATH_ST']   = '-F%s'
    v['RPATH_ST'] = '-Wl,-rpath,%s'
    
    # Default frameworks to always link
    v['FRAMEWORK'] = [ 'Foundation', 'Cocoa', 'Carbon', 'CoreServices', 'ApplicationServices', 'AudioUnit', 'CoreAudio', 'AppKit', 'ForceFeedBack', 'IOKit', 'OpenGL', 'GameController' ]
    
    # Default libraries to always link
    v['LIB'] = ['c', 'm', 'pthread', 'ncurses']
    
    # Setup compiler and linker settings  for mac bundles
    v['CFLAGS_MACBUNDLE'] = v['CXXFLAGS_MACBUNDLE'] = '-fpic'
    v['LINKFLAGS_MACBUNDLE'] = [
        '-dynamiclib',
        '-undefined', 
        'dynamic_lookup'
        ]
        
    # Set the path to the current sdk
    sdk_path = subprocess.check_output(["xcrun", "--sdk", "macosx", "--show-sdk-path"]).strip()
    v['CFLAGS'] += [ '-isysroot' + sdk_path, ]
    v['CXXFLAGS'] += [ '-isysroot' + sdk_path, ]
    v['LINKFLAGS'] += [ '-isysroot' + sdk_path, ]

    # Since we only support a single darwin target (clang-64bit), we specify all tools directly here    
    v['AR'] = 'ar'
    v['CC'] = 'clang'
    v['CXX'] = 'clang++'
    v['LINK'] = v['LINK_CC'] = v['LINK_CXX'] = 'clang++'
    
@conf
def load_debug_darwin_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin configurations for
    the 'debug' configuration
    """
    v = conf.env
    conf.load_darwin_common_settings()

@conf
def load_profile_darwin_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin configurations for
    the 'debug' configuration
    """
    v = conf.env
    conf.load_darwin_common_settings()

@conf
def load_performance_darwin_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin configurations for
    the 'debug' configuration
    """
    v = conf.env
    conf.load_darwin_common_settings()

@conf
def load_release_darwin_settings(conf):
    """
    Setup all compiler and linker settings shared over all darwin configurations for
    the 'debug' configuration
    """
    v = conf.env
    conf.load_darwin_common_settings()


@feature('cprogram', 'cxxprogram')
@after_method('post_command_exec')
def add_post_build_mac_command(self):
    """
    Function to add a post build method if we are creating a mac launcher and
    add copy jobs for any 3rd party libraries that are used by the launcher to
    the executable directory.
    """
    if getattr(self, 'mac_launcher', False) and self.env['PLATFORM'] != 'project_generator':
        self.bld.add_post_fun(add_transfer_jobs_for_mac_launcher)

        # Screen the potential libpaths first
        libpaths = []
        for libpath in self.libpath:
            full_libpath = os.path.join(self.bld.root.abspath(), libpath)
            if not os.path.exists(full_libpath):
                Logs.warn('[WARN] Unable to find library path {} to copy libraries from.')
            else:
                libpaths.append(libpath)

        for lib in self.lib:
            for libpath in libpaths:
                libpath_node = self.bld.root.find_node(libpath)
                library = libpath_node.ant_glob("lib" + lib + ".dylib")
                if library:
                    self.copy_files(library[0])

import os, shutil
def add_transfer_jobs_for_mac_launcher(self):
    """
    Function to move CryEngine libraries that the mac launcher uses to the
    project .app Contents/MacOS folder where they need to be for the executable
    to run
    """
    output_folders = self.get_output_folders(self.env['PLATFORM'], self.env['CONFIGURATION'])
    for folder in output_folders:
        files_to_copy = folder.ant_glob("*.dylib")
        for project in self.get_enabled_game_project_list():
            executable_name = self.get_executable_name(project)
            destination_folder = folder.abspath() + "/" + executable_name + ".app/Contents/MacOS/"
            for file in files_to_copy:
                # Some of the libraries/files we copy over may have read-only
                # permissions. If they do the copy2 will fail. Delete the file
                # first then do the copy. If it does not exist then we will
                # have no issues doing the copy so we can pass on the
                # exception.
                try:
                    os.remove(destination_folder + file.name)
                except:
                    pass
                     
                shutil.copy2(file.abspath(), destination_folder)


class update_to_use_rpath(Task):
    '''
    Updates dependent libraries to use rpath if they are not already using
    rpath or are referenced by an absolute path 
    '''
    color = 'CYAN'

    def runnable_status(self):
        if Task.runnable_status(self) == ASK_LATER:
            return ASK_LATER

        source_lib = self.inputs[0]
        depenedent_libs = self.get_dependent_libs(source_lib.abspath())
        for dependent_name in depenedent_libs:
            if dependent_name and dependent_name != source_lib.name and re.match("[@/]", dependent_name) is None:
                return RUN_ME

        return SKIP_ME

    def run(self):
        self.update_lib_to_use_rpath(self.inputs[0])
        return 0

    def update_lib_to_use_rpath(self, source_lib):
        self.update_dependent_libs_to_use_rpath(source_lib)

        # Always update the id. If we are operating on an executable then this does nothing.
        # Otherwise the library gets the rpath in its id, which is required on macOS to
        # properly load the library when using the rpath system.
        self.update_lib_id_to_use_rpath(source_lib)

    def update_dependent_libs_to_use_rpath(self, source_lib):
        depenedent_libs = self.get_dependent_libs(source_lib.abspath())
        for dependent_name in depenedent_libs:
            if dependent_name and dependent_name != source_lib.name and re.match("[@/]", dependent_name) is None:
                self.update_lib_to_reference_dependent_by_rpath(source_lib, dependent_name)

                dependent_lib = source_lib.parent.make_node(dependent_name)

                try:
                    dependent_lib.chmod(FILE_PERMISSIONS)
                    self.update_lib_to_use_rpath(dependent_lib)
                except:
                    # File most likely does not exist so just continue on processing the other dependencies
                    Logs.debug("update_rpath: dependent lib does not exist: %s" % (dependent_lib.abspath()))
                    pass

    def get_dependent_libs(self, lib_path_name):
        otool_command = "otool -L '%s' | cut -d ' ' -f 1 | grep .*dylib$" % (lib_path_name) 
        output = subprocess.check_output(otool_command, shell=True)
        depenedent_libs = re.split("\s+", output.strip())
        Logs.debug("update_rpath: dependent libs: %s" % (depenedent_libs))
        return depenedent_libs

    def update_lib_to_reference_dependent_by_rpath(self, source_lib, dependent):
        install_name_command = "install_name_tool -change %s @rpath/%s '%s'" % (dependent, dependent, source_lib.abspath())
        Logs.debug("update_rpath: executing %s" % (install_name_command))
        output = subprocess.check_output(install_name_command, shell=True)

    def update_lib_id_to_use_rpath(self, source_lib):
        install_name_command = "install_name_tool -id @rpath/%s '%s'" % (source_lib.name, source_lib.abspath())
        Logs.debug("update_rpath: updating lib id - %s" % (install_name_command))
        subprocess.check_output(install_name_command, shell=True)


@feature('c', 'cxx')
@after_method('copy_3rd_party_binaries')
def update_3rd_party_libs_to_use_rpath(self):
    '''
    Update any 3rd party libraries that we copy over to use rpath and reference
    any dependencies that don't have an aboslute path or rpath already to use
    rpath as well. This is necessary since we copy 3rd party libraries at the
    beginning of a build and could overwrite any rpath changes we made before.
    '''
    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    third_party_artifacts = None

    if self.env['COPY_3RD_PARTY_ARTIFACTS']:
        # only interested in shared libraries that are being copied
        third_party_artifacts = [artifact for artifact in self.env['COPY_3RD_PARTY_ARTIFACTS'] if "dylib" in artifact.name]

    if third_party_artifacts:

        current_platform = self.bld.env['PLATFORM']
        current_configuration = self.bld.env['CONFIGURATION']

        # Iterate through all target output folders
        for target_node in self.bld.get_output_folders(current_platform, current_configuration, self):

            # Determine the final output directory the library is going to be in
            output_sub_folder = getattr(self, 'output_sub_folder', None)
            if output_sub_folder:
                output_path_node = target_node.make_node(output_sub_folder)
            else:
                output_path_node = target_node

            for source_node in third_party_artifacts:
                source_filename = os.path.basename(source_node.abspath())
                target_full_path = output_path_node.make_node(source_filename)

                try:
                    target_full_path.chmod(FILE_PERMISSIONS)
                    self.create_task('update_to_use_rpath', target_full_path)
                except:
                    pass
         

@feature('cxxshlib', 'cxxprogram')
@after_method('set_link_outputs')
def add_rpath_update_tasks(self):
    '''
    Update the libraries and executables that we build to use rpath and reference
    any dependencies that don't have an aboslute path or rpath already to use
    rpath as well. The update task should not run often as most libraries
    should already use rpath and be linked to libraries that are using rpath as
    well.  There are a few libraries though that may be missed so this will
    ensure in the end all libraries and executables will load on macOS
    '''
    if 'darwin' not in self.env['PLATFORM']:
        return

    if not getattr(self, 'link_task', None):
        return

    for src in self.link_task.outputs:
        Logs.debug("update_rpath: creating task to update {}".format(src.abspath()))
        self.create_task('update_to_use_rpath', src)


@conf
def register_darwin_external_ly_identity(self, configuration):

    # Do not regsiter as an external library if the source exists
    if os.path.exists(self.Path('Code/Tools/LyIdentity/wscript')):
        return

    platform = 'mac'

    if configuration not in ('Debug', 'Release'):
        raise WafError("Invalid configuration value {}", configuration)

    target_platform = 'darwin'
    ly_identity_base_path = self.CreateRootRelativePath('Tools/InternalSDKs/LyIdentity')
    include_path = os.path.join(ly_identity_base_path, 'include')
    stlib_path = os.path.join(ly_identity_base_path, 'lib', platform, configuration)

    self.register_3rd_party_uselib('LyIdentity_static',
                                   target_platform,
                                   includes=[include_path],
                                   libpath=[stlib_path],
                                   lib=['libLyIdentity_static.a'])

@conf
def register_darwin_external_ly_metrics(self, configuration):

    # Do not regsiter as an external library if the source exists
    if os.path.exists(self.Path('Code/Tools/LyMetrics/wscript')):
        return

    platform = 'mac'

    if configuration not in ('Debug', 'Release'):
        raise WafError("Invalid configuration value {}", configuration)

    target_platform = 'darwin'
    ly_identity_base_path = self.CreateRootRelativePath('Tools/InternalSDKs/LyMetrics')
    include_path = os.path.join(ly_identity_base_path, 'include')
    stlib_path = os.path.join(ly_identity_base_path, 'lib', platform, configuration)

    self.register_3rd_party_uselib('LyMetricsShared_static',
                                   target_platform,
                                   includes=[include_path],
                                   libpath=[stlib_path],
                                   lib=['libLyMetricsShared_static.a'])

    self.register_3rd_party_uselib('LyMetricsProducer_static',
                                   target_platform,
                                   includes=[include_path],
                                   libpath=[stlib_path],
                                   lib=['libLyMetricsProducer_static.a'])
