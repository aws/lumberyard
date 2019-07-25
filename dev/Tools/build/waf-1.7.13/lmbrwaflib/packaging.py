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
import plistlib
import re
import shutil
import socket
import subprocess
import time

from gems import Gem
from lmbr_install_context import LmbrInstallContext
from lumberyard_sdks import get_dynamic_lib_extension, get_platform_lib_prefix
from qt5 import QT5_LIBS
from utils import *
from build_configurations import PLATFORM_MAP
from contextlib import closing

from waflib import Build, Errors, Logs, Utils
from waflib.Scripting import run_command
from waflib.Task import Task, ASK_LATER, RUN_ME, SKIP_ME


################################################################
#                            Globals                           #
DEFAULT_RC_JOB = os.path.join('Code', 'Tools', 'RC', 'Config', 'rc', 'RCJob_Generic_MakePaks.xml')
#                                                              #
################################################################


def get_python_path(ctx):
    python_cmd = 'python'

    if Utils.unversioned_sys_platform() == "win32":
        python_cmd = '"{}"'.format(ctx.path.find_resource('Tools/Python/python.cmd').abspath())

    return python_cmd

def run_subprocess(command_args, working_dir = None, as_shell = False, fail_on_error = False):
    exe_name = os.path.basename(command_args[0])
    command = ' '.join(command_args)
    Logs.debug('package: Running command - {}'.format(command))

    try:
        cmd = command if as_shell else command_args
        output = subprocess.check_output(cmd, stderr = subprocess.STDOUT, cwd = working_dir, shell = as_shell)
        Logs.debug('package: Output = {}'.format(output.rstrip()))
        return True

    except subprocess.CalledProcessError as process_err:
        error_message = (
            '"{}" exited with non-zero return code, {}\n'
            'Command: {}\n'
            'Output: {}').format(exe_name, process_err.returncode, command, process_err.output)

    except Exception as err:
        error_message = (
            'An exception was thrown while running "{}"\n'
            'Command: {}\n'
            'Exception: {}').format(exe_name, command, err)

    if fail_on_error:
        raise Errors.WafError(error_message)
    else:
        Logs.debug('package: {}'.format(error_message))

    return False


def run_xcode_build(ctx, target, platform, config):
    if not (ctx.is_apple_platform(platform) and ctx.is_option_true('run_xcode_for_packaging')):
        Logs.debug('package: Not running xcode because either we are not on a macOS platform or the command line option disabled it')
        return True

    Logs.info('Running xcodebuild for {}'.format(target))

    if 'darwin' in platform:
        platform = 'mac'

    get_project_name = getattr(ctx, 'get_{}_project_name'.format(platform), lambda : '')
    project_name = get_project_name()

    xcodebuild_cmd = [
        'xcodebuild',
        '-project', '{}.xcodeproj'.format(project_name),
        '-target', target,
        '-configuration', config,
        '-quiet',
        'RUN_WAF_BUILD=NO'
    ]

    run_subprocess(xcodebuild_cmd, fail_on_error=True)


def get_resource_compiler_path(ctx):
    output_folders = ctx.get_standard_host_output_folders()
    if not output_folders:
        ctx.fatal('[ERROR] Unable to determine possible binary directories for host {}'.format(Utils.unversioned_sys_platform()))

    rc_search_paths = [ os.path.join(output_node.abspath(), 'rc') for output_node in output_folders ]
    try:
        rc_path = ctx.find_program('rc', path_list = rc_search_paths, silent_output = True)
        return ctx.root.make_node(rc_path)
    except:
        ctx.fatal('[ERROR] Failed to find the Resource Compiler in paths: {}'.format(rc_search_paths))


def run_rc_job(ctx, game, job, assets_platform, source_path, target_path, is_obb = False, num_threads = 0, zip_split = False, zip_maxsize = 0, verbose = 0):
    rc_path = get_resource_compiler_path(ctx)

    default_rc_job = os.path.join(ctx.get_engine_node().abspath(), DEFAULT_RC_JOB)

    job_file = (job or default_rc_job)

    if not os.path.isabs(job_file):
        rc_root = rc_path.parent
        rc_job = rc_root.find_resource(job_file)
        if not rc_job:
            ctx.fatal('[ERROR] Unable to locate the RC job in path - {}/{}'.format(rc_root.abspath(), job_file))
        job_file = rc_job.path_from(ctx.launch_node())
        
    if not os.path.isfile(job_file):
        ctx.fatal('[ERROR] Unable to locate the RC job in path - {}'.format(job_file))

    rc_cmd = [
        '"{}"'.format(rc_path.abspath()),
        '/game={}'.format(game.lower()),
        '/job="{}"'.format(job_file),
        '/p={}'.format(assets_platform),
        '/src="{}"'.format(source_path),
        '/trg="{}"'.format(target_path)
    ]

    if verbose:
        rc_cmd.extend([
            '/verbose={}'.format(verbose)
        ])

    if zip_split:
        rc_cmd.extend([
            '/zip_sizesplit={}'.format(zip_split)
        ])

    if zip_maxsize:
        rc_cmd.extend([
            '/zip_maxsize={}'.format(zip_maxsize)
        ])

    if num_threads:
        rc_cmd.extend([
            # by default run single threaded since anything higher than one is effectively ingored on mac
            # while pc produces a bunch of "thread not joinable" errors
            '/threads={}'.format(num_threads)
        ])

    if is_obb:
        package_name = ctx.get_android_package_name(game)
        app_version_number = ctx.get_android_version_number(game)

        rc_cmd.extend([
            '/obb_pak=main.{}.{}.obb'.format(app_version_number, package_name),
            '/obb_patch_pak=patch.{}.{}.obb'.format(app_version_number, package_name)
        ])

    # Invoking RC as non-shell exposes how rigid it's command line parsing is...
    return run_subprocess(rc_cmd, working_dir = ctx.launch_dir, as_shell = True)


def get_shader_compiler_ip_and_port(ctx):
    shader_cache_gen_cfg = ctx.engine_node.find_node('shadercachegen.cfg')

    if shader_cache_gen_cfg:
        shader_cache_gen_cfg_contents = shader_cache_gen_cfg.read()

        def parse_config(search_string, default):
            result = ''
            try:
                result = re.search(search_string, shader_cache_gen_cfg_contents, re.MULTILINE).group(1).strip()
            except:
                pass
            if not result:
                return default
            return result
        
        shader_compiler_ip = parse_config('^\s*r_ShaderCompilerServer\s*=\s*((\d+[.]*)*)', '127.0.0.1')
        shader_compiler_port = parse_config('^\s*r_ShaderCompilerPort\s*=\s*((\d)*)', '61453')
        asset_processor_shader_compiler = parse_config('^\s*r_AssetProcessorShaderCompiler\s*=\s*(\d+)', '0')
    else:
        ctx.fatal('[ERROR] Failed to find shadercachegen.cfg in the engine root directory {}'.format(ctx.engine_node.abspath()))

    # If we're connecting to the shader compiler through the Asset Processor, we need to check if remote_ip is set
    if asset_processor_shader_compiler == '1':
        remote_ip = ctx.get_bootstrap_remote_ip()
        if shader_compiler_ip.find("127.0.0.") != -1:
            if remote_ip.find("127.0.0.") == -1:
                Logs.debug('package: Using Asset Processor at {} to route requests to shader compiler'.format(remote_ip))
                shader_compiler_ip = remote_ip

    return shader_compiler_ip, int(shader_compiler_port)


def is_shader_compiler_valid(ctx):
    shader_compiler_ip, shader_compiler_port = get_shader_compiler_ip_and_port(ctx)

    with closing(socket.socket(socket.AF_INET, socket.SOCK_STREAM)) as sock:
        for i in range(5):
            result = sock.connect_ex((shader_compiler_ip, shader_compiler_port))
            if result == 0:
                Logs.debug('package: Successfully connected to {}'.format(shader_compiler_ip))

                #send the size of the xml+1 packed as a Q uint_8
                import struct
                xml = '<?xml version="1.0" ?><X Identify=1 />'
                xml_len = len(xml) + 1
                format = 'Q'
                request = struct.pack(format, xml_len)
                sock.sendall(request)

                #send the xml packed as a s i.e. string of xml_len
                format = str(xml_len) + 's'
                request = struct.pack(format, xml)
                sock.sendall(request)

                # make socket non blocking
                sock.setblocking(0)

                #recv the response, if any
                response = []
                data=''
                begin = time.time()
                while True:
                    #5 second timeout after last recv
                    if time.time() - begin > 5:
                        break

                    try:
                        data = sock.recv(4096)
                        if data:
                            response.append(data)
                            begin = time.time()
                    except:
                        pass

                if 'ShaderCompilerServer' in str(response):
                    Logs.debug('package: {} is a shader compiler server'.format(shader_compiler_ip))
                    return True
                else:
                    Logs.error('[ERROR] {} does not identify as a shader compiler server!!!'.format(shader_compiler_ip))
                    return False
            else:
                time.sleep(2)

    Logs.error('[ERROR] Unable to connect to shader compiler at {}'.format(shader_compiler_ip))
    return False


def get_shader_cache_gen_path(ctx):
    output_folders = [output.abspath() for output in ctx.get_standard_host_output_folders()]
    if not output_folders:
        ctx.fatal('[ERROR] Unable to determine possible binary directories for host {}'.format(Utils.unversioned_sys_platform()))

    try:
        return ctx.find_program('ShaderCacheGen', path_list = output_folders, silent_output = True)
    except:
        ctx.fatal('[ERROR] Failed to find the ShaderCacheGen in paths: {}'.format(output_folders))

def generate_game_paks(ctx, game, job, assets_platform, source_path, target_path, is_obb=False, num_threads=0, zip_split=False, zip_maxsize=0, verbose=0):
    Logs.info('[INFO] Generate game paks...')
    timer = Utils.Timer()
    try:
        ret = run_rc_job(ctx, game, job, assets_platform, source_path, target_path, is_obb, num_threads, zip_split, zip_maxsize, verbose)
        if ret:
            Logs.info('[INFO] Finished generating game paks... ({})'.format(timer))
        else:
            Logs.error('[ERROR] Generating game paks failed.')
    except:
        Logs.error('[ERROR] Generating game paks exception.')

def get_shader_list(ctx, game, assets_platform, shader_type, shader_list_file=None):
    Logs.info('[INFO] Get the shader list from the shader compiler server...')
    timer = Utils.Timer()

    if not is_shader_compiler_valid(ctx):
        ctx.fatal('[ERROR] Unable to connect to the remote shader compiler to get shaders list. Please check shadercachegen.cfg in the engine root directory to ensure r_ShaderCompilerServer is set to the correct IP address')

    shader_cache_gen_path = get_shader_cache_gen_path(ctx)
    get_shader_list_script = ctx.engine_node.find_resource('Tools/PakShaders/get_shader_list.py')
    command_args = [
        get_python_path(ctx),
        '"{}"'.format(get_shader_list_script.abspath()),
        '{}'.format(game),
        '{}'.format(assets_platform),
        '{}'.format(shader_type),
        '{}'.format(os.path.basename(os.path.dirname(shader_cache_gen_path))),
        '-g "{}"'.format(ctx.launch_dir)
    ]

    if shader_list_file:
        command_args += ['-s {}'.format(shader_list_file)]

    command = ' '.join(command_args)
    Logs.info('[INFO] Running command - {}'.format(command))
    try:
        subprocess.check_call(command, shell = True)
        Logs.info('[INFO] Finished getting the shader list from the shader compiler server...({})'.format(timer))
    except subprocess.CalledProcessError:
        Logs.error('[ERROR] Failed to get shader list for {}'.format(shader_type))

    return

def generate_shaders_pak(ctx, game, assets_platform, shader_type, shader_list_file=None, shaders_pak_dir=None):
    Logs.info('[INFO] Generate Shaders...')
    timer = Utils.Timer()

    if not is_shader_compiler_valid(ctx):
        ctx.fatal('[ERROR] Unable to connect to the remote shader compiler for generating shaders. Please check shadercachegen.cfg in the engine root directory to ensure r_ShaderCompilerServer is set to the correct IP address')

    shader_cache_gen_path = get_shader_cache_gen_path(ctx)
    gen_shaders_script = ctx.engine_node.find_resource('Tools/PakShaders/gen_shaders.py')
    command_args = [
        get_python_path(ctx),
        '"{}"'.format(gen_shaders_script.abspath()),
        '{}'.format(game),
        '{}'.format(assets_platform),
        '{}'.format(shader_type),
        '{}'.format(os.path.basename(os.path.dirname(shader_cache_gen_path))),
        '-g "{}"'.format(ctx.launch_dir),
        '-e "{}"'.format(ctx.engine_node.abspath())
    ]

    if not run_subprocess(command_args, as_shell=True):
        ctx.fatal('[ERROR] Failed to generate {} shaders'.format(shader_type))

    pak_shaders_script = ctx.engine_node.find_resource('Tools/PakShaders/pak_shaders.py')

    shaders_pak_dir = ctx.launch_node().make_node('build/{}/{}'.format(assets_platform, game).lower())
    if os.path.isdir(shaders_pak_dir.abspath()):
        shaders_pak_dir.delete()
    shaders_pak_dir.mkdir()

    shaders_source = ctx.launch_node().make_node('Cache/{}/{}/user/cache/shaders/cache'.format(game, assets_platform))

    command_args = [
        get_python_path(ctx),
        '"{}"'.format(pak_shaders_script.abspath()),
        '"{}"'.format(shaders_pak_dir.abspath()),
        '-s {}'.format(shader_type),
        '-r "{}"'.format(shaders_source.abspath())
    ]
    if not run_subprocess(command_args, as_shell=True):
        ctx.fatal('[ERROR] Failed to pack {} shaders'.format(shader_type))

    Logs.info('[INFO] Finished Generate Shaders...({})'.format(timer))

    return shaders_pak_dir


def clear_user_cache(assets_node):
    """
    Based on the assets node, search and delete the 'user' cache subfolder
    :param assets_node: The assets node to search for the user cache
    """
    if not assets_node:
        return
    # The 'user' subfolder is expected to be directly under the assets directory specified by the assets_node
    user_subpath = os.path.join(assets_node.abspath(), 'user')
    
    if os.path.isdir(user_subpath):
        # Attempt to remove the 'user' folder only if it exists
        try:
            shutil.rmtree(user_subpath)
        except OSError as e:
            # Any permission issue cannot be mitigated at this point, we have to warn the user that the cache
            # could not be deleted at least
            Logs.warn('[WARN] Unable to clear the user cache folder "{}": {}'.format(user_subpath, e))


class package_task(Task):
    """
    Package an executable and any resources and assets it uses into a platform specific format.

    Extended `Task`
    """

    color = 'CYAN'
    optional = False

    def __init__(self, *k, **kw):
        """
        Construct the package task

        :param bld: Reference to the  current WAF context.
        :type bld: Context
        :param source_exe: Path to the source executable to be packaged, maps to the task inputs
        :type source_exe: waflib.Node
        :param output_root: Root folder of the final package, maps to the task outputs
        :type output_root: waflib.Node
        :param executable_task_gen: The WAF task generator for building the executable.
        :type executable_task_gen: waflib.task_gen
        :param spec: The spec to use to get module dependencies.
        :type spec: string
        :param gem_types: [Optional] Types of gem modules to include as dependencies. Defaults to a list
                          containing GameModule if not specified
        :type gem_types: list of Gem.Module.Type
        :param resources: [Optional] Files that should be copied to the resource directory. Resource
                          directory is determined by calling get_resource_node
        :type resources: list of strings
        :param dir_resources: [Optional] Directories that contain resources required for the executable
                              (such as QtLibs). These directories will either be linked or copied into
                              the location of the executable
        :type dir_resources: list of strings
        :param assets_node: [Optional] Source assets to be included in the package
        :type assets_node: waflib.Node
        :param finalize_func: [Optional] A function to execute when the package task has finished.
                              The package context and node containing the destination of the executable
                              is passed into the function.
        :type finalize_func: function
        """
        super(package_task, self).__init__(self, *k, **kw)

        self.bld = kw['bld']

        self.set_inputs(kw['source_exe'])
        self.executable_task_gen = kw['executable_task_gen']
        self.spec = kw['spec']
        self.include_all_libs = (not self.spec or self.spec == 'all')
        self.standard_out = self.bld.get_output_folders(self.bld.platform, self.bld.config)[0]
        self.gem_types = kw.get('gem_types', [Gem.Module.Type.GameModule])
        self.resources = kw.get('resources', [])
        self.dir_resources = kw.get('dir_resources', [])
        self.assets_source = kw.get('assets_node', None)
        self.finalize_func = kw.get('finalize_func', lambda *args: None)

        self.set_outputs(kw['output_root'])
        self.binaries_out = self.root_out.make_node(self.bld.get_binaries_sub_folder())
        self.resources_out = self.root_out.make_node(self.bld.get_resources_sub_folder())
        self.assets_out = self.root_out.make_node(self.bld.get_assets_sub_folder())

    def get_src_exe(self):
        # inputs zero will be guaranteed to be the executable (or entry point) node
        return self.inputs[0]
    source_exe = property(get_src_exe, None)

    def get_root_out(self):
        return self.outputs[0]
    root_out = property(get_root_out, None)

    def runnable_status(self):
        if len(self.inputs) == 1 and self.assets_source:
            self.inputs.extend(self.assets_source.ant_glob('**/*', excl=['user/']))

        return super(package_task, self).runnable_status()

    def scan(self):
        """
        Overridden scan to check for extra dependencies.
        """
        dep_nodes = self.bld.node_deps.get(self.uid(), [])
        return (dep_nodes, [])

    def run(self):
        Logs.debug('package: packaging {} to destination {}'.format(self.source_exe.abspath(), self.root_out.abspath()))

        self.process_dependencies()
        self.process_executable()
        self.process_resources()
        self.process_assets()

        if self.bld.is_apple_platform(self.bld.platform):
            run_xcode_build(self.bld, self.executable_task_gen.name, self.bld.platform, self.bld.config)

    def post_run(self):
        super(package_task, self).post_run()
        self.bld.node_deps[self.uid()] = list(self.dependencies)

    def process_dependencies(self):
        """
        This function inspects the task_generator for its dependencies and
        if include_spec_dependencies has been specified to include modules that
        are specified in the spec and are potentially part of the project.
        """
        ctx = self.bld

        if ctx.is_build_monolithic(ctx.platform, ctx.config):
            self.dependencies = []
            return

        self.dependencies = get_dependencies_recursively_for_task_gen(ctx, self.executable_task_gen)
        self.dependencies.update(get_spec_dependencies(ctx, self.spec, self.gem_types))

        # get_dependencies_recursively_for_task_gen will not pick up all the
        # gems so if we want all libs add all the gems to the
        # dependencies as well
        if self.include_all_libs:
            for gem in GemManager.GetInstance(ctx).gems:
                for module in gem.modules:
                    gem_module_task_gen = ctx.get_tgen_by_name(module.target_name)
                    self.dependencies.update(get_dependencies_recursively_for_task_gen(ctx, gem_module_task_gen))

    def process_executable(self):
        source_root = self.source_exe.parent
        source_dep_nodes = list(set([
            source_root,
            self.standard_out
        ]))

        self.binaries_out.mkdir()
        self.bld.install_files(self.binaries_out.abspath(), self.source_exe, chmod=Utils.O755, postpone=False)

        if self.include_all_libs:
            self.bld.symlink_libraries(source_root, self.binaries_out.abspath())
        else:
            self.bld.symlink_dependencies(self.dependencies, source_dep_nodes, self.binaries_out.abspath())

        self.finalize_func(self.bld, self.binaries_out)

    def process_qt(self):
        """
        Process Qt libraries for packaging for macOS. 

        This function will copy the Qt framework/libraries that an application
        needs into the specific location for App bundles (Frameworks directory)
        and perform any cleanup on the copied framework to conform to Apple's
        framework bundle structure. This is required so that App bundles can
        be properly code signed.
        """
        if not self.bld.is_mac_platform(self.bld.platform):
            return

        # Don't need the process_resources method to process the qtlibs folder
        # since we are handling it
        self.dir_resources.remove('qtlibs')
        executable_dest_node = self.binaries_out

        output_folder_node = self.standard_out
        qt_plugin_source_node = output_folder_node.make_node("qtlibs/plugins")
        qt_plugins_dest_node = executable_dest_node.make_node("qtlibs/plugins")

        # To be on the safe side check if the destination qtlibs is a link and
        # unlink it before we creat the plugins copy/link
        if os.path.islink(qt_plugins_dest_node.parent.abspath()):
            os.unlink(qt_plugins_dest_node.parent.abspath())

        self.bld.create_symlink_or_copy(qt_plugin_source_node, qt_plugins_dest_node.abspath(), postpone=False)

        qt_libs_source_node = output_folder_node.make_node("qtlibs/lib")

        # Executable destination node will be something like
        # Application.app/Contents/MacOS. The parent will be Contents, which
        # needs to contain the Frameworks folder according to macOS Framework
        # bundle structure 
        frameworks_node = executable_dest_node.parent.make_node("Frameworks")
        frameworks_node.mkdir()

        def post_copy_cleanup(dst_framework_node):
            # Apple does not like any file in the top level directory of an
            # embedded framework. In 5.6 Qt has Perl scripts for their build in the
            # top level directory so we will just delete them from the embedded
            # framework since we won't be building anything.
            pearl_files = dst_framework_node.ant_glob("*.prl")
            for file in pearl_files:
                file.delete()
        
        # on macOS there is not a clean way to get Qt dependencies on itself,
        # so we have to scan the lib using otool and then add any of those Qt
        # dependencies to our set.

        qt_frameworks_to_copy = set()

        qt5_vars = Utils.to_list(QT5_LIBS)
        for i in qt5_vars:
            uselib = i.upper()
            if uselib in self.dependencies:
                # QT for darwin does not have '5' in the name, so we need to remove it
                darwin_adjusted_name = i.replace('Qt5','Qt')
                framework_name = darwin_adjusted_name + ".framework"
                src = qt_libs_source_node.make_node(framework_name).abspath()

                if os.path.exists(src):
                    qt_frameworks_to_copy.add(framework_name)

                    # otool -L will generate output like this:
                    #     @rpath/QtWebKit.framework/Versions/5/QtWebKit (compatibility version 5.6.0, current version 5.6.0)
                    # cut -d ' ' -f 1 will slice the line by spaces and returns the first field. That results in: @rpath/QtWebKit.framework/Versions/5/QtWebKit
                    # grep @rpath will make sure we only have QtLibraries and not system libraries
                    # cut -d '/' -f 2 slices the line by '/' and selects the second field resulting in: QtWebKit.framework
                    otool_command = "otool -L '%s' | cut -d ' ' -f 1 | grep @rpath.*Qt | cut -d '/' -f 2" % (os.path.join(src, darwin_adjusted_name)) 
                    output = subprocess.check_output(otool_command, shell=True)
                    qt_dependent_libs = re.split("\s+", output.strip())

                    for lib in qt_dependent_libs:
                        qt_frameworks_to_copy.add(lib)

        for framework_name in qt_frameworks_to_copy:
            src_node = qt_libs_source_node.make_node(framework_name)
            src = src_node.abspath()
            dst = frameworks_node.make_node(framework_name).abspath()
            if os.path.islink(dst):
                os.unlink(dst)
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            Logs.info('Copying Qt Framework {} to {}'.format(src, dst))
            self.bld.create_symlink_or_copy(src_node, dst)
            if not os.path.islink(dst):
                post_copy_cleanup(frameworks_node.make_node(framework_name))

    def process_resources(self):
        resources_dest_node = self.resources_out
        resources = getattr(self, 'resources', None)
        if resources:
            resources_dest_node.mkdir()
            self.bld.install_files(resources_dest_node.abspath(), resources)

        if 'qtlibs' in self.dir_resources:
            self.process_qt()

        for res_dir in self.dir_resources:
            Logs.debug('package: extra directory to link/copy into the package is: {}'.format(res_dir))
            self.bld.create_symlink_or_copy(self.source_exe.parent.make_node(res_dir), self.binaries_out.make_node(res_dir).abspath(), postpone=False)

    def process_assets(self):
        """
        Packages any assets if supplied
        """
        if not self.assets_source:
            return

        Logs.debug('package: Assets source      = {}'.format(self.assets_source.abspath()))
        Logs.debug('package: Assets destination = {}'.format(self.assets_out.abspath()))

        Logs.info('Placing assets into folder {}'.format(self.assets_out.abspath()))
        self.bld.create_symlink_or_copy(self.assets_source, self.assets_out.abspath(), postpone=False)


class PackageContext(LmbrInstallContext):
    fun = 'package'
    group_name = 'packaging'

    def should_copy_and_not_link(self):
        if not hasattr(self, '_copy_files'):
            self._copy_files = (Utils.is_win32
                                or self.platform in ('appletv', 'ios')
                                or self.paks_required
                                or self.is_option_true('copy_assets'))
        return self._copy_files
    copy_over_symlink = property(should_copy_and_not_link, None)

    def preprocess_tasks(self):
        """
        For apple targets, the xcode project is required to be generated before any
        of the packaging tasks can be executed
        """
        # Generating the xcode project should only be done on macOS and if we actually have something to package (len(group) > 0)
        group = self.get_group(self.group_name)
        if len(group) > 0 and self.is_option_true('run_xcode_for_packaging') and self.is_apple_platform(self.platform):
            Logs.debug('package: checking for xcode project... ')
            platform = self.platform
            if 'darwin' in platform:
                platform = 'mac'

            # Check if the Xcode solution exists. We need it to perform bundle
            # stuff (processing Info.plist and icon assets...)
            project_name_and_location = '/{}/{}.xcodeproj'.format(getattr(self.options, platform + '_project_folder', None), getattr(self.options, platform + '_project_name', None))
            if not os.path.exists(self.path.abspath() + project_name_and_location):
                Logs.debug('package: running xcode_{} command to generate the project {}'.format(platform, self.path.abspath() + project_name_and_location))
                run_command('xcode_' + platform)

    def package_game(self, **kw):
        """ 
        Packages a game up by creating and configuring a package_task object.

        This method should be called by wscript files when they want to package a
        game into a final form. See package_task.__init__ for more information
        about the various keywords that can be passed in to configure the packaging
        task.

        :param destination: [Optional] Override the output path for the package
        :type destination: string
        """
        if not self.is_valid_package_request(**kw):
            return

        game = self.project
        if game not in self.get_enabled_game_project_list():
            Logs.warn('[WARN] The game project specified in bootstrap.cfg - {} - is not in the '
                        'enabled game projects list.  Skipping packaging...'.format(game))
            return

        assets_dir = self.assets_cache_path
        assets_node = self.get_assets_cache_node()
        assets_platform = self.assets_platform

        if assets_node is None:
            assets_node = self.launch_node().make_node(assets_dir)

            if self.copy_over_symlink:
                output_folders = [folder.name for folder in self.get_standard_host_output_folders()]
                Logs.error('[ERROR] There is no asset cache to read from at "{}". Please run AssetProcessor or '
                              'AssetProcessorBatch from {} with "{}" assets enabled in the '
                              'AssetProcessorPlatformConfig.ini'.format(assets_dir, '|'.join(output_folders), assets_platform))
                return
            else:
                Logs.warn('[WARN] Asset source location {} does not exist on the file system. Creating the assets source folder.'.format(assets_dir))
                assets_node.mkdir()

        # clear all stale transient files (e.g. logs, shader source, etc.) from the source asset cache
        self.clear_user_cache(assets_node)

        if self.use_paks():
            if self.use_vfs():
                self.fatal('[ERROR] Cannot use VFS when the --{}-paks is set to "True".  Please set remote_filesystem=0 in bootstrap.cfg'.format(self.platform_alias))

            paks_node = assets_node.parent.make_node('{}_paks'.format(assets_platform))
            paks_node.mkdir()

            # clear all stale transient files (e.g. logs, shader source/paks, etc.) from the pak file cache
            self.clear_user_cache(paks_node)
            self.clear_shader_cache(paks_node)

            Logs.info('Generating the core pak files for {}'.format(game))
            run_rc_job(self, game, job=None, assets_platform=assets_platform, source_path=assets_node.abspath(), target_path=paks_node.abspath())

            if self.paks_required:
                Logs.info('Generating the shader pak files for {}'.format(game))
                shaders_pak_dir = generate_shaders_pak(self, game, assets_platform, self.shaders_type)

                shader_paks = shaders_pak_dir.ant_glob('*.pak')
                if not shader_paks:
                    self.fatal('[ERROR] No shader pak files were found after running the pak_shaders command')

                self.install_files(os.path.join(paks_node.abspath(), game.lower()), shader_paks, postpone=False)

            assets_node = paks_node

        kw['assets_node'] = assets_node
        kw['spec'] = 'game_and_engine'

        self.create_package_task(**kw)

    def package_game_legacy(self, **kw):
        """
        Creates a package task using the old feature method
        """
        if self.is_platform_and_config_valid(**kw):
            self(features='package_{}'.format(self.platform), group='packaging')

    def package_tool(self, **kw):
        """ 
        Packages a tool up by creating and configuring a package_task object.

        This method should be called by wscript files when they want to package a
        game into a final form. See package_task.__init__ for more information
        about the various keywords that can be passed in to configure the packaging
        task. Note that option to use pak files does not pertain to tools and is
        explicitly set to false by this function.

        :param destination: [Optional] Override the output path for the package
        :type destination: string
        :param include_all_libs: [Optional] Force the task to include all libs that are in the same 
                                 directory as the built executable to be part of the package
        :type include_all_libs: boolean
        """
        if not self.is_valid_package_request(**kw):
            return

        kw['spec'] = 'all' if kw.get('include_all_libs', False) else self.options.project_spec

        self.create_package_task(**kw)

    def is_valid_package_request(self, **kw):
        """ Returns if the platform and configuration specified for the package_task match what the package context has been created for"""
        executable_name = kw.get('target', None)
        if not executable_name:
            Logs.warn('[WARN] Skipping package because no target was specified.')
            return False

        if not self.is_platform_and_config_valid(**kw):
            return False

        task_gen_name = kw.get('task_gen_name', executable_name)

        if self.options.project_spec:
            modules_in_spec = self.loaded_specs_dict[self.options.project_spec]['modules']
            if task_gen_name not in modules_in_spec:
                Logs.debug('package: Skipping packaging {} because it is not part of the spec {}'.format(task_gen_name, self.options.project_spec))
                return False

        if self.options.targets and executable_name not in self.options.targets.split(','):
            Logs.debug('package: Skipping packaging {} because it is not part of the specified targets {}'.format(executable_name, self.options.targets))
            return False

        return True

    def create_package_task(self, **kw):
        executable_name = kw['target']
        task_gen_name = kw.get('task_gen_name', executable_name)

        Logs.debug('package: create_package_task {}'.format(task_gen_name))

        kw['bld'] = self # Needed for when we build the task

        executable_task_gen = self.get_tgen_by_name(task_gen_name)
        kw['executable_task_gen'] = executable_task_gen

        source_output_folder = getattr(executable_task_gen,'output_folder', None)
        if not source_output_folder:
            source_output_folder = self.get_output_folders(self.platform, self.config)[0].name
        source_output_node = self.srcnode.make_node(source_output_folder)

        executable_source_node = source_output_node.make_node(executable_name)
        if not os.path.exists(executable_source_node.abspath()):
            self.fatal('[ERROR] Failed to find the executable in path {}.  '
                        'Run the build command for {}_{} to generate it'.format(executable_source_node.abspath(), self.platform, self.config))
        kw['source_exe'] = executable_source_node

        destination_node = kw.get('destination', source_output_node)
        kw['output_root'] = destination_node.make_node(self.convert_to_package_name(executable_name))

        new_task = package_task(env=self.env, **kw)
        self.add_to_group(new_task, 'packaging')

    def clear_user_cache(self, node):
        clear_user_cache(node)

    def clear_shader_cache(self, node):
        if not node:
            return
        shadercache_paks = node.ant_glob('**/shadercache*.pak')
        for pak in shadercache_paks:
            pak.delete()

    def convert_to_package_name(self, target):
        if self.is_apple_platform(self.platform):
            return '{}.app'.format(target)
        elif target in self.get_enabled_game_project_list():
            return self.get_executable_name(target)
        return target

    def get_binaries_sub_folder(self):
        if self.is_mac_platform(self.platform):
            return 'Contents/MacOS'
        return ''

    def get_resources_sub_folder(self):
        if self.is_mac_platform(self.platform):
            return 'Contents/Resources'
        return ''

    def get_assets_sub_folder(self):
        if self.is_mac_platform(self.platform):
            return 'Contents/Resources/assets'
        elif self.is_apple_platform(self.platform):
            return 'assets'
        return ''

    def symlink_libraries(self, source, destination): 
        """ Creates a symbolic link for libraries.
        
        An ant_glob is executed on the source node using "*" + result of get_dynamic_lib_extension to determine all the
        libraries that need to be linked into the destination. If the package is being built for a release configuration
        or the platform does not support symbolic links a copy will be made of the library.

        :param source: Source of the libraries
        :type source: waflib.Node
        :param destination: Location/path to create the link (or copy) of any libraries found in Source
        :type destination: String 
        """

        lib_extension = get_dynamic_lib_extension(self)
        Logs.debug('package: Copying files with pattern {} from {} to {}'.format('*'+lib_extension, source.abspath(), destination))
        lib_files = source.ant_glob("*" + lib_extension)
        for lib in lib_files:
            self.create_symlink_or_copy(lib, destination, postpone=False)

    def symlink_dependencies(self, dependencies, dependency_source_location_nodes, executable_dest_path):
        """ Creates a symbolic link dependencies.
        
        If the package is being built for a release configuration or the platform does not support symbolic links a
        copy will be made of the library.

        :param dependencies: Dependencies to link/copy to executable_dest_path
        :type dependencies: list of strings
        :param dependency_source_location_nodes: Source locations to find dependencies
        :type destination: List of Node objects 
        :param executable_dest_path: Path where the executable/dependencies should be placed
        :type executable_dest_path: String 
        """

        if not isinstance(dependency_source_location_nodes, list): 
            dependency_source_location_nodes = [dependency_source_location_nodes]

        Logs.debug('package: linking module dependencies {}'.format(dependencies))

        lib_prefix = get_platform_lib_prefix(self)
        lib_extension = get_dynamic_lib_extension(self)
        
        for dependency in dependencies:
            depend_nodes = []
            source_node = None
            for source_location in dependency_source_location_nodes:
                depend_nodes = source_location.ant_glob(lib_prefix + dependency + lib_extension, ignorecase=True) 
                if depend_nodes and len(depend_nodes) > 0:
                    source_node = source_location
                    break

                elif 'AWS_CPP_SDK_ALL' == dependency:
                    # This is a special dependency that requests all AWS libraries to be dependencies.
                    Logs.debug('package: Processing AWS_CPP_SDK_ALL. Getting all AWS libs...')
                    depend_nodes = source_location.ant_glob("libaws-cpp*.dylib", ignorecase=True) 
                    if depend_nodes and len(depend_nodes) > 0:
                        source_node = source_location
                        break

                elif 'AWS' in dependency:
                    # AWS libraries in use/use_lib use _ to separate the name,
                    # but the actual library uses '-'. Transform the name and try
                    # the glob again to see if we pick up the dependent library
                    Logs.debug('package: Processing AWS lib so changing name to lib*{}*.dylib'.format(dependency.replace('_','-')))
                    depend_nodes = source_location.ant_glob('lib*' + dependency.replace('_','-') + '*.dylib', ignorecase=True)
                    if depend_nodes and len(depend_nodes) > 0:
                        source_node = source_location
                        break

            if len(depend_nodes) > 0:
                Logs.debug('package: found dependency {} in {}'.format(depend_nodes, source_node.abspath(), depend_nodes))
                for dependency_node in depend_nodes:
                    self.create_symlink_or_copy(dependency_node, executable_dest_path, postpone=False)
            else:
                Logs.debug('package: Could not find the dependency {}. It may be a static library, in which case this '
                           'can be ignored, or a directory that contains dependencies is missing'.format(dependency))

    def create_symlink_or_copy(self, source_node, destination_path, **kwargs):
        """ 
        Creates a symlink or copies a file/directory to a destination.

        If the configuration is release or the python platform does not support symbolic links (like windows) this
        method will instead copy the source to the destination. Any keyword arguments that install_files or symlink_as
        support can also be passed in to this method (they will get passed to the underlying install_files or symlink_as
        calls without modification).

        :param source_node: Source to be linked/copied
        :param destination_path: Location to link/copy the source
        :type destination_path: string
        """

        source_path_and_name = source_node.abspath()
        Logs.debug('package: create_symlink_or_copy called with source: {} destination: {} args: {}'.format(source_path_and_name, destination_path, kwargs))
        try:
            if os.path.isdir(source_path_and_name):
                Logs.debug('package: -> path is a directory')

                # need to delete everything to make sure the copy is fresh
                # otherwise can get old artifacts laying around
                if os.path.islink(destination_path):
                    Logs.debug('package: -> removing previous link...')
                    remove_junction(destination_path)
                elif os.path.isdir(destination_path): 
                    Logs.debug('package: -> removing previous directory...')
                    shutil.rmtree(destination_path)

                if self.copy_over_symlink:
                    shutil.copytree(source_path_and_name, destination_path, symlinks=True)
                else:
                    # Make sure that the parent directory exists otherwise the junction_directory will fail
                    (parent, child) = os.path.split(destination_path)
                    if not os.path.exists(parent):
                        os.makedirs(parent)
                    junction_directory(source_path_and_name, destination_path)

            else:
                Logs.debug('package: -> path is a file')
                if self.copy_over_symlink:
                    Logs.debug('package: -> need to copy so calling install_files for the file {} {}'.format(destination_path, source_node.abspath()))
                    self.install_files(destination_path, [source_node], **kwargs)
                else:
                    kwargs.pop('chmod', None) # optional chmod arg is only needed when install_files is used
                    dest_path = '{}/{}'.format(destination_path, source_node.name)
                    Logs.debug('package: -> calling symlink_as {}'.format(dest_path))
                    self.symlink_as(dest_path, source_path_and_name, **kwargs)

        except Exception as err:
            Logs.debug('package:: -> got an exception {}'.format(err))


for platform_name, platform in PLATFORM_MAP.items():
    for configuration in platform.get_configuration_names():
        platform_config_key = '{}_{}'.format(platform_name, configuration)

        # Create new class to execute package command with variant
        class_attributes = {
            'cmd'       : 'package_{}'.format(platform_config_key),
            'variant'   : platform_config_key,
            'config'    : configuration,
            'platform'  : platform_name,
        }

        subclass = type('{}{}PackageContext'.format(platform_name.title(), configuration.title()), (PackageContext,), class_attributes)
