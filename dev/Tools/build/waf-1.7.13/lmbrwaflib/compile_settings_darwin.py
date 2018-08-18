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
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME, SUCCESS
from waflib.Errors import WafError
import subprocess, re, os

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
    v['FRAMEWORK'] = [ 'Foundation', 'Cocoa', 'Carbon', 'CoreServices', 'ApplicationServices', 'AudioUnit', 'CoreAudio', 'AppKit', 'ForceFeedback', 'IOKit', 'OpenGL', 'GameController' ]
    
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


class update_to_use_rpath(Task):
    '''
    Updates dependent libraries to use rpath if they are not already using
    rpath or are referenced by an absolute path 
    '''
    color = 'CYAN'
    queued_libs = set()
    processed_libs = set()
    lib_dependencies = {}

    def runnable_status(self):
        self_status = Task.runnable_status(self)

        # Check only for ASK_LATER and not != RUN_ME. The runnable_status
        # method will return SKIP_ME for pre-built libraries that are only
        # being copied but we still need to run the rpath update on the library
        if self_status == ASK_LATER: 
            return self_status

        if self.use_link_task:
            # Need to use hasrun to determine if the task was runned or skipped
            # as runnable_status is always returning RUN_ME after the task has
            # been run. 

            # If the link_task status is not success then the link task was
            # either skipped or we had an error so no reason to run. We don't
            # have to worry about NOT_RUN state since we set this task to only
            # run after the link_task has been run when we create it below.  
            if self.generator.link_task.hasrun != SUCCESS:
                return SKIP_ME

        source_lib = self.inputs[0]
        if source_lib.abspath() in self.processed_libs or source_lib in self.queued_libs:
            return SKIP_ME

        self.queued_libs.add(source_lib)

        return RUN_ME

    def run(self):
        self.update_lib_to_use_rpath(self.inputs[0])
        return 0

    def update_lib_to_use_rpath(self, source_lib):
        if source_lib.abspath() in self.processed_libs:
            Logs.debug("update_rpath: %s --> source_lib already processed" % (self.inputs[0].name))
            return
             
        if not os.path.exists(source_lib.abspath()):
            Logs.warn("Could not find library %s, skipping the update to have it use rpath." % (self.inputs[0].abspath()))
            return
             
        Logs.debug("update_rpath: %s --> processing..." % (self.inputs[0].abspath()))

        # Make sure that we can write to the source lib since we will be
        # changing various references to libraries and its ID
        source_lib.chmod(FILE_PERMISSIONS)

        self.update_dependent_libs_to_use_rpath(source_lib)

        # Always update the id. If we are operating on an executable then this does nothing.
        # Otherwise the library gets the rpath in its id, which is required on macOS to
        # properly load the library when using the rpath system.
        self.update_lib_id_to_use_rpath(source_lib)

        self.processed_libs.add(source_lib.abspath())

    def update_dependent_libs_to_use_rpath(self, source_lib):
        for dependent_name in self.get_dependent_libs(source_lib):
            if dependent_name != source_lib.name and re.match("[@/]", dependent_name) is None:
                Logs.debug("update_rpath: %s - processing dependent_name %s" % (self.inputs[0].name, dependent_name))
                self.update_lib_to_reference_dependent_by_rpath(source_lib, dependent_name)

                dependent_lib = source_lib.parent.make_node(dependent_name)
                self.update_lib_to_use_rpath(dependent_lib)

    def get_dependent_libs(self, lib_path_node):
        if lib_path_node.name in self.lib_dependencies:
            return self.lib_dependencies[lib_path_node.name]

        otool_command = "otool -L '%s' | cut -d ' ' -f 1 | grep .*dylib$" % (lib_path_node.abspath()) 
        output = subprocess.check_output(otool_command, shell=True)
        dependent_libs = re.split("\s+", output.strip())

        self.lib_dependencies[lib_path_node.name] = dependent_libs
        Logs.debug("update_rpath: %s - dependent libs: %s" % (self.inputs[0].name, dependent_libs))
        return dependent_libs

    def update_lib_to_reference_dependent_by_rpath(self, source_lib, dependent):
        install_name_command = "install_name_tool -change %s @rpath/%s '%s'" % (dependent, dependent, source_lib.abspath())
        output = subprocess.check_output(install_name_command, shell=True)

    def update_lib_id_to_use_rpath(self, source_lib):
        install_name_command = "install_name_tool -id @rpath/%s '%s'" % (source_lib.name, source_lib.abspath())
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
                    task = self.create_task('update_to_use_rpath', target_full_path)
                    task.use_link_task = False
                except:
                    pass
         

@feature('cxxshlib', 'cxxprogram')
@after_method('set_link_outputs')
def add_rpath_update_tasks(self):
    '''
    Update the libraries and executables that we build to use rpath and reference
    any dependencies that don't have an aboslute path or rpath already to use
    rpath as well. 
    '''

    if 'darwin' not in self.env['PLATFORM']:
        return

    if not getattr(self, 'link_task', None):
        return

    for src in self.link_task.outputs:
        task = self.create_task('update_to_use_rpath', src)
        task.set_run_after(self.link_task)
        task.use_link_task = True


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
