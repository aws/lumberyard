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

from waflib import Logs, Utils
from waflib.Configure import conf
from waflib.TaskGen import feature, after_method
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME
from waflib.Errors import WafError

import subprocess
import re
import os


@conf
def register_darwin_external_ly_identity(ctx, configuration):
    # Do not regsiter as an external library if the source exists
    if os.path.exists(ctx.Path('Code/Tools/LyIdentity/wscript')):
        return

    platform = 'mac'

    if configuration not in ('Debug', 'Release'):
        raise WafError("Invalid configuration value {}", configuration)

    target_platform = 'darwin'
    ly_identity_base_path = ctx.CreateRootRelativePath('Tools/InternalSDKs/LyIdentity')
    include_path = os.path.join(ly_identity_base_path, 'include')
    stlib_path = os.path.join(ly_identity_base_path, 'lib', platform, configuration)
    shlib_path = os.path.join(ly_identity_base_path, 'bin', platform, configuration)

    ctx.register_3rd_party_uselib('LyIdentity_shared',
                                   target_platform,
                                   includes=[include_path],
                                   defines=['LINK_LY_IDENTITY_DYNAMICALLY'],
                                   importlib=['libLyIdentity_shared.dylib'],
                                   sharedlibpath=[shlib_path],
                                   sharedlib=['libLyIdentity_shared.dylib'])

    ctx.register_3rd_party_uselib('LyIdentity_static',
                                   target_platform,
                                   includes=[include_path],
                                   libpath=[stlib_path],
                                   lib=['libLyIdentity_static.a'])


@conf
def register_darwin_external_ly_metrics(ctx, configuration):
    # Do not regsiter as an external library if the source exists
    if os.path.exists(ctx.Path('Code/Tools/LyMetrics/wscript')):
        return

    platform = 'mac'

    if configuration not in ('Debug', 'Release'):
        raise WafError("Invalid configuration value {}", configuration)

    target_platform = 'darwin'
    ly_identity_base_path = ctx.CreateRootRelativePath('Tools/InternalSDKs/LyMetrics')
    include_path = os.path.join(ly_identity_base_path, 'include')
    stlib_path = os.path.join(ly_identity_base_path, 'lib', platform, configuration)
    shlib_path = os.path.join(ly_identity_base_path, 'bin', platform, configuration)

    ctx.register_3rd_party_uselib('LyMetricsShared_shared',
                                  target_platform,
                                  includes=[include_path],
                                  defines=['LINK_LY_METRICS_DYNAMICALLY'],
                                  importlib=['libLyMetricsShared_shared.dylib'],
                                  sharedlibpath=[shlib_path],
                                  sharedlib=['libLyMetricsShared_shared.dylib'])

    ctx.register_3rd_party_uselib('LyMetricsShared_static',
                                  target_platform,
                                  includes=[include_path],
                                  libpath=[stlib_path],
                                  lib=['libLyMetricsShared_static.a'])

    ctx.register_3rd_party_uselib('LyMetricsProducer_shared',
                                  target_platform,
                                  includes=[include_path],
                                  defines=['LINK_LY_METRICS_PRODUCER_DYNAMICALLY'],
                                  importlib=['libLyMetricsProducer_shared.dylib'],
                                  sharedlibpath=[shlib_path],
                                  sharedlib=['libLyMetricsProducer_shared.dylib'])

    ctx.register_3rd_party_uselib('LyMetricsProducer_static',
                                  target_platform,
                                  includes=[include_path],
                                  libpath=[stlib_path],
                                  lib=['libLyMetricsProducer_static.a'])




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
        if self_status == ASK_LATER: 
            return ASK_LATER

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
        source_lib.chmod(Utils.O755)

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
                    target_full_path.chmod(Utils.O755)
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
        self.create_task('update_to_use_rpath', src)


