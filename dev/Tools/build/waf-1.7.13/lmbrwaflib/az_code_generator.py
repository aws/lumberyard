#!/usr/bin/env python
# encoding: utf-8

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

from waflib.TaskGen import feature, after_method, before_method, taskgen_method
from waflib.Context import BOTH
from waflib.Configure import conf
from waflib import Node, Task, Utils, Logs, Errors, Options
from binascii import hexlify
from collections import defaultdict
from pipes import quote
import os
import json
import waflib.Build
import threading
import itertools

#  Code Generator Settings
code_generator_ignore_includes = False
code_generator_suppress_errors_as_warnings = False
code_generator_verbose = False
command_length_when_windows_fails_to_execute = 8192

# Module for our utility
module = "AZCodeGenerator"

# Add our task data to be saved between builds
waflib.Build.SAVED_ATTRS.append('azcg')

# Create a lock object to be used for az_code_gen task node access
# This must be used before any calls to make_node, find_node or similar methods during task execution
# These methods create nodes and simultaneous task invocation that attempts to create the same node will error
task_node_access_lock = threading.Lock()


def get_input_dir_node(tg):
    """
    Gets the input dir from a task generator.

    If 'az_code_gen_input_dir' is set, use that. Otherwise, use the project's root directory.
    """
    input_dir = getattr(tg, 'az_code_gen_input_dir', None)
    if input_dir:
        if os.path.isabs(input_dir):
            input_dir = tg.bld.root.make_node(input_dir)
        else:
            input_dir = tg.bld.srcnode.make_node(input_dir)

    else:
        input_dir = tg.path
    return input_dir


@conf
def azcg_output_dir_node(ctx, target_name=None, additional_subpath=None):
    """
    Gets the output dir from a task generator.
    """
    if target_name:
        output_dir = ctx.bldnode.make_node('azcg/{}'.format(target_name))
    else:
        output_dir = ctx.bldnode.make_node('azcg')
    if additional_subpath:
        output_dir = output_dir.make_node(additional_subpath)
    return output_dir

@taskgen_method
def get_azcg_output_dir_node(tg, additional_subpath=None, tg_name_override=None):
    """
    Gets the output dir from a task generator.
    """
    tg_name = tg_name_override if tg_name_override else tg.name
    # Apply any override for the az_code_gen target, otherwise fall back to the default behavior
    return getattr(tg, 'az_code_gen_override_target', azcg_output_dir_node(tg.bld, tg_name, additional_subpath))

@taskgen_method
def get_azcg_output_dir_path(tg, additional_subpath=None, tg_name_override=None):
    return get_azcg_output_dir_node(tg, additional_subpath, tg_name_override).abspath()

@feature('az_code_gen')
@before_method('process_use')
def add_codegen_includes(self):
    for attr in ('includes', 'export_includes'):
        if not hasattr(self, attr):
            setattr(self, attr, list())
    include_path_to_inject = get_azcg_output_dir_node(self)
    self.includes.append(include_path_to_inject)
    self.export_includes.append(include_path_to_inject)


@feature('az_code_gen')
@after_method('apply_incpaths')
def create_code_generator_tasks(self):
    # Skip during project generation
    if self.bld.env['PLATFORM'] == 'project_generator':
        return

    # promote raw entries to list
    if isinstance(getattr(self, 'az_code_gen', []), dict):
        self.az_code_gen = [self.az_code_gen]

    # compute deps
    azcg_dep_nodes = []
    azcg_dir = self.env['CODE_GENERATOR_PATH'][0]
    azcg_dir_node = self.bld.root.find_node(azcg_dir)
    if azcg_dir_node:
        azcg_dep_nodes = azcg_dir_node.ant_glob('**/*')
    else:
        Logs.warn('az_code_gen: Unable to find azcg directory. Code Generator tasks will not have the utility/scripts as dependencies')

    # this script is a dependency
    script_node = self.bld.engine_node.make_node('Tools/build/waf-1.7.13/lmbrwaflib/az_code_generator.py')
    azcg_dep_nodes.append(script_node)

    for az_code_gen_pass in getattr(self, 'az_code_gen', []):
        # See if we have any scripts
        code_generator_scripts = az_code_gen_pass.get('scripts', [])
        if not code_generator_scripts:
            Logs.warn(
                'az_code_gen feature enabled but no scripts were specified. '
                'No code generation performed for target {}'.format(self.target))
            return

        code_gen_arguments = az_code_gen_pass.get('arguments', [])
        if isinstance(code_gen_arguments, str):
            code_gen_arguments = [code_gen_arguments]

        code_gen_options = az_code_gen_pass.get('options', [])
        if isinstance(code_gen_options, str):
            code_gen_options = [code_gen_options]

        code_gen_input = az_code_gen_pass.get('files', [])
        if not code_gen_input:
            Logs.warn(
                'az_code_gen feature enabled but no files were specified. '
                'No code generation performed for target {}'.format(self.target))
            return

        code_gen_override_output = az_code_gen_pass.get('override_output', None)

        # Create one task per input file/list
        for input_item in code_gen_input:
            # Auto promote non-lists to lists
            if not isinstance(input_item, list):
                input_file_list = [input_item]
            else:
                input_file_list = input_item

            create_az_code_generator_task(self, input_file_list, code_generator_scripts, code_gen_arguments, code_gen_options, azcg_dep_nodes, code_gen_override_output)


def create_az_code_generator_task(self, input_file_list, code_generator_scripts, code_gen_arguments, code_gen_options, azcg_dep_nodes, code_gen_override_output):
    input_dir_node = get_input_dir_node(self)
    input_nodes = [in_file if isinstance(in_file, waflib.Node.Node) else input_dir_node.make_node(clean_path(in_file)) for in_file in input_file_list]

    # Create a code gen task.
    # We would simply do "new_task = self.create_task('az_code_gen', input_nodes)" had we no need
    # to worry about build ordering.  Instead, we add to az_code_gen_group (tbd: a better name?)
    new_task = az_code_gen(env=self.env.derive(), generator=self)
    new_task.set_inputs(input_nodes)
    self.bld.add_to_group(new_task, 'az_code_gen_group')

    new_task.path = self.path
    new_task.input_dir = input_dir_node
    new_task.output_dir = code_gen_override_output or get_azcg_output_dir_node(self)
    for script in code_generator_scripts:
        script_node = self.path.find_or_declare(script)
        if script_node is None:
            raise Errors.WafError('[ERROR] Unable to find node for script {}'.format(script))
        new_task.script_nodes.append(script_node)
    new_task.includes = self.includes_nodes
    new_task.defines = self.env['DEFINES']
    new_task.azcg_deps = azcg_dep_nodes
    for code_gen_argument in code_gen_arguments:
        new_task.add_argument(code_gen_argument)

    if 'PrintOutputRedirectionFile' in code_gen_options:
        new_task.capture_and_print_error_output()

    if self.bld.is_option_true('use_debug_code_generator'):
        Logs.warn("Using DEBUG AZCodeGenerator!")

        path = new_task.env.CODE_GENERATOR_PATH[0]
        if path.endswith('profilenosym'):
            new_task.env.CODE_GENERATOR_PATH = [path[:-len('profilenosym')] + 'debug']

        az_code_gen_exe = "AzCodeGeneratorD.exe"
        full_debug_path = os.path.join(new_task.env['CODE_GENERATOR_PATH'][0],az_code_gen_exe)
        if os.path.exists(full_debug_path):
            new_task.env['CODE_GENERATOR_EXECUTABLE'] = "AzCodeGeneratorD.exe"
        else:
            raise Errors.WafError('[ERROR] Debug AzCodeGenerator executable ({}) was not found.  Make sure to build it first if you want to run the code generator in debug mode'.format(full_debug_path))

    # Pre-compute arguments for task since doing this during run() causes multi-thread issues in Node
    new_task.prepare_task()

def clean_path(path):
    return path.replace('\\', '/')

def hash_node_list(list, up):
    for item in list:
        if isinstance(item, waflib.Node.Node):
            up(item.abspath().encode())
        else:
            up(item)

# ported from msvcdeps.py
def path_to_node(base_node, path, cached_nodes):
    # Take the base node and the path and return a node
    # Results are cached because searching the node tree is expensive
    # The following code is executed by threads, it is not safe, so a lock is needed...
    if getattr(path, '__hash__'):
        node_lookup_key = (base_node, path)
    else:
        # Not hashable, assume it is a list and join into a string
        node_lookup_key = (base_node, os.path.sep.join(path))
    try:
        task_node_access_lock.acquire()
        node = cached_nodes[node_lookup_key]
    except KeyError:
        node = base_node.find_resource(path)
        cached_nodes[node_lookup_key] = node
    finally:
        task_node_access_lock.release()
    return node

class az_code_gen(Task.Task):
    color = 'CYAN'

    def __init__(self, *k, **kw):
        super(az_code_gen, self).__init__(*k, **kw)

        self.more_tasks = []
        self.input_dir = None
        self.output_dir = None
        self.script_nodes = []
        self.includes = []
        self.defines = []
        self.argument_list = []
        self.error_output_file_node = None
        self.registered_dependencies = []
        self.azcg_deps = []
        self.generated_output = []
        self.profile = False
        # Ensure we have a 'azcg' attribute to cache task info for future builds
        if not isinstance(getattr(self.generator.bld, 'azcg', None), dict):
            self.generator.bld.azcg = defaultdict(dict)
        # Ensure the task generator has a lock to manage INCPATHS
        if not hasattr(self.generator, 'task_gen_access_lock'):
            self.generator.task_gen_access_lock = threading.Lock()
        # Ensure the task generator has a lock to manage link task additions
        if not hasattr(self.generator, 'task_gen_link_lock'):
            self.generator.task_gen_link_lock = threading.Lock()


    def __str__(self):
        return 'AZ Code Generation for {} -> Processing {}\n'.format(self.generator.target, ', '.join(str(input) for input in self.inputs))

    def uid(self):
        try:
            return self.uid_
        except AttributeError:
            m = Utils.md5()
            up = m.update

            # Be sure to add any items here that change how code gen runs, this needs to be unique!
            # Ensure anything here will not change over the life of the task
            up(self.path.abspath().encode())
            up(self.input_dir.abspath().encode())
            up(self.output_dir.abspath().encode())
            hash_node_list(self.inputs, up)
            hash_node_list(self.script_nodes, up)
            hash_node_list(self.includes, up)
            hash_node_list(self.defines, up)
            hash_node_list(self.argument_list, up)
            hash_node_list(self.azcg_deps, up)

            self.uid_ = m.digest()
            return self.uid_

    def azcg_get(self, key, default_value=None):
        assert isinstance(key, str)
        return self.generator.bld.azcg[self.uid()].get(key, default_value)

    def azcg_set(self, key, val):
        assert isinstance(key, str)
        self.generator.bld.azcg[self.uid()][key] = val

    def azcg_append_unique(self, key, val):
        assert isinstance(key, str)
        vals = self.azcg_get(key, [])
        assert isinstance(vals, list)
        if val not in vals:
            vals.append(val)
        self.azcg_set(key, vals)

    def add_argument(self, argument):
        """
        Add an argument to the argument list
        """
        self.argument_list.append(argument)

    def capture_and_print_error_output(self):
        error_output_file_path = 'CodeGenErrorOutput/error_output_{}.log'.format(hexlify(self.uid()))
        self.error_output_file_node = self.generator.bld.bldnode.find_or_declare(error_output_file_path)

    def register_output_file(self, output_path, should_add_to_build):
        # Store this output object for later use since we will need to be on the main thread for node access
        self.generated_output.append({'output_path': output_path, 'should_add_to_build' : should_add_to_build})

    def process_generated_output(self):
        # This should only ever be executed on the main thread! Node operations cannot happen during run()
        for generated_output in self.generated_output:
            output_path = generated_output['output_path']
            should_add_to_build = generated_output['should_add_to_build']

            # Nodes won't work with absolute paths, so we have to remove the build path from
            # the given output file path. Given path is unicode, make it str to match Node.
            output_path = os.path.relpath(str(output_path), start=self.generator.bld.bldnode.abspath())
            with task_node_access_lock:
                output_node = self.generator.bld.bldnode.find_node(output_path)

            if output_node is None:
                raise Errors.WafError('[ERROR] az_code_gen: Unable to find generated file as node {}'.format(output_path))

            self.outputs.append(output_node)

            if should_add_to_build:
                # Add to persistent link list
                self.azcg_append_unique('link_inputs', output_node)
                # Perform actual add to link task
                self.add_link_task(output_node)

    def register_dependency_file(self, path):
        if not (path.endswith('NUL') or path.endswith('null')):
            self.registered_dependencies.append(path)

    def add_link_task(self, node_to_link):
        # Using modified version of example here:
        # https://github.com/waf-project/waf/blob/7b7531b0c6d0598033bea608ffc3c8e335434a6d/docs/book/examples/scenarios_unknown/mytool.py
        try:
            task_hook = self.generator.get_hook(node_to_link)
        except Errors.WafError:
            raise Errors.WafError(
                '[ERROR] az_code_gen: Created file {} marked for "should add to build" '
                'is not buildable.'.format(node_to_link.path_from(self.generator.bldnode)))

        created_task = task_hook(self.generator, node_to_link)

        # Shove /Fd flags into codegen meta-tasks, this is similar to logic in mscv_helper's
        # set_pdb_flags. We compute PDB file path and add the requisite /Fd flag
        # This enables debug symbols for code outputted by azcg
        if 'msvc' in (self.generator.env.CC_NAME, self.generator.env.CXX_NAME):

            # The created_task from the original generator will not be able to go through the
            # 'verify_compiler_options_msvc' function at this point, so we will manually verify
            # the compiler options here (to strip out conflicting flags)
            verify_options_common(created_task.env)

            # Not having PDBs stops CL.exe working for precompiled header when we have VCCompiler set to true for IB...
            # When DISABLE_DEBUG_SYMBOLS_OVERRIDE doesn't exist in the dictionary it returns []
            # which will results to false in this check.
            if self.bld.is_option_true('generate_debug_info') or self.generator.env['DISABLE_DEBUG_SYMBOLS_OVERRIDE']:
                pdb_folder = self.generator.path.get_bld().make_node(str(self.generator.target_uid))
                pdb_cxxflag = '/Fd{}'.format(pdb_folder.abspath())

                created_task.env.append_unique('CFLAGS', pdb_cxxflag)
                created_task.env.append_unique('CXXFLAGS', pdb_cxxflag)
        
        link_task = getattr(self.generator, 'link_task', None)
        if not link_task:
            link_task = getattr(self.bld, 'monolithic_link_task', None)
        if link_task:
            link_task.set_run_after(created_task)  # Compile our .cpp before we link.

            # link_task is a shared resource that lives on the generator. Use a lock and a separate list
            # to ensure that the append order is consistent
            with self.generator.task_gen_link_lock:
                if not hasattr(self.generator, 'task_gen_link_inputs'):
                    self.generator.task_gen_link_inputs = []

                if(created_task.outputs[0] not in self.generator.task_gen_link_inputs):
                    self.generator.task_gen_link_inputs.append(created_task.outputs[0])
                    self.generator.task_gen_link_inputs.sort(key=lambda x: x.name)

                    for output in self.generator.task_gen_link_inputs:
                        try:
                            idx = link_task.inputs.index(output)
                            del link_task.inputs[idx:]
                            break
                        except:
                            continue
                    link_task.inputs += self.generator.task_gen_link_inputs
        else:
            # If we ever have a use case where link_task is inappropriate (non-C-family lang?),
            # then we should do "self.more_tasks.append(created_task)" in those cases.
            raise Errors.WafError('[ERROR] az_code_gen: Created file {} marked for "should add to build" '
                       'was not added to a link task.'.format(
                           node_to_link.path_from(self.generator.bld.bldnode)))
        return True

    def propagate_azcg_incpaths(self, azcg_paths):
        """
        Performs a thread safe and consistently ordered update of this task's generator's INCPATHS
        :param azcg_paths: List of paths from this azcg task's output nodes that should be appended to INCPATHS
        """
        with self.generator.task_gen_access_lock:

            for azcg_path in azcg_paths:
                self.generator.env.append_unique('AZCG_INCPATHS', azcg_path)

            # AZCG_INCPATHS can be affected by multiple azcg tasks, clean it out and re-add for ordering consistency
            self.generator.env['AZCG_INCPATHS'].sort()
            for path in self.generator.env['AZCG_INCPATHS']:
                if path in self.generator.env['INCPATHS']:
                    self.generator.env['INCPATHS'].remove(path)
            for path in self.generator.env['AZCG_INCPATHS']:
                self.generator.env.append_unique('INCPATHS', path)

    def write_argument_list_to_file(self):
        """
        Writes argument_list to a file
        :return: (True, <argument file>) on success; (False, '') on failure
        """
        argument_file_path = 'CodeGenArguments/{}_{}.args'.format(self.inputs[0].name.replace(".", "_"), hexlify(self.uid()))
        argument_file_node = self.generator.bld.bldnode.find_or_declare(argument_file_path)
        if os.name is 'nt' and len(argument_file_node.abspath()) >= 260:
            Logs.error('Unable to write argument file for code gen, path will be unable to write due to MAX_PATH limit at 260 characters. Please use a shorter root path.  Length: {} - Path: {}'.format(len(argument_file_node.abspath()), argument_file_node.abspath()))
            return False, ''
        try:
            argument_file_node.write('\n'.join(self.argument_list))
        except:
            Logs.error(
                'az_code_gen: Failed to write argument file {}'.format(argument_file_node.abspath()))
            return False, ''
        return True, argument_file_node.abspath()

    def handle_code_generator_output_errors(self, code_gen_output):
        try:
            json_object = json.loads(code_gen_output)
            for output_object in json_object:
                if code_generator_verbose and output_object['type'] == 'info':
                    Logs.debug('az_code_gen: {}'.format(output_object['info']))
                if output_object['type'] == 'error':
                    Logs.error('{} - az_code_gen task error'.format(output_object['error']))
        except ValueError as value_error:
            # If we get output that isn't JSON, it means Clang errored before
            # the code generator gained control. Likely invalid commandline arguments.
            raise Errors.WafError('az_code_gen: Failed to json.loads output with error "{}" - output string was:\n{}'.format(str(value_error), code_gen_output))

    def handle_code_generator_output(self, code_gen_output):
        """
        Decode json object and process return from generator
        :param code_gen_output: json string
        :return True on success, False on failure
        """
        try:
            json_object = json.loads(code_gen_output)
            errors_reported = False
            for output_object in json_object:
                if output_object['type'] == 'info':
                    output = output_object['info']
                    Logs.debug('az_code_gen: {}'.format(output))
                    if (self.profile and output.startswith('Profile')):
                        Logs.debug('az_code_gen: ' + output)
                elif output_object['type'] == 'error':
                    Logs.error('{} - az_code_gen task error'.format(output_object['error']))
                    errors_reported = True
                elif output_object['type'] == 'generated_file':
                    self.register_output_file(output_object['file_name'],
                                              output_object['should_be_added_to_build'])
                elif output_object['type'] == 'dependency_file':
                    self.register_dependency_file(str(output_object['file_name']))
                else:
                    Logs.error('az_code_gen: Unknown output json type returned from Code Generator. Type is: {} - Raw output: {}'.format(output_object['type'], code_gen_output))
                    errors_reported = True

            # Fail the task if errors were reported
            if errors_reported:
                return False

            # Add local folder of each output node to include path of the task_gen and store path off to pickle for future runs
            azcg_paths = self.azcg_get('AZCG_INCPATHS', [])
            for output_node in self.outputs:
                # This check is here to ensure that tasks that were written out that had None outputs will be skipped.
                # The origin of this problem should have been solved by returning after the None checking during register output
                if output_node is None:
                    Logs.warn('az_code_gen: Task output has a None entry, skipping!')
                    continue
                output_path = output_node.parent.abspath()
                if output_path not in azcg_paths:
                    azcg_paths.append(output_path)

            # Append any additional paths relative to the output directory found in export includes
            output_dir_node = get_azcg_output_dir_node(self.generator)
            for export_include in self.generator.export_includes:
                if isinstance(export_include, waflib.Node.Node) and export_include.is_child_of(output_dir_node):
                    export_path = export_include.abspath()
                    if export_path not in azcg_paths:
                        azcg_paths.append(export_path)

            self.azcg_set('AZCG_INCPATHS', azcg_paths)
            self.propagate_azcg_incpaths(azcg_paths)

            return True
        except ValueError as value_error:
            # If we get output that isn't JSON, it means Clang errored before
            # the code generator gained control. Likely invalid commandline arguments.
            Logs.error('az_code_gen: Failed to json.loads output with error "{}" - output string was:\n{}'.format(str(value_error), code_gen_output))
            import traceback
            import sys
            tb_list = traceback.extract_tb(sys.exc_traceback)
            for filename, lineno, name, line in tb_list:
                Logs.error(
                    '{}({}): error {}: in {}: {}'.format(filename, lineno,
                                                         value_error.__class__.__name__,
                                                         name, line))
            filename, lineno, _, _ = tb_list[-1]
            Logs.error('{}({}): error {}: {}'.format(filename, lineno,
                                                     value_error.__class__.__name__,
                                                     str(value_error)))
            return False

    def print_error_output(self):
        Logs.error('Error output stored in {}:'.format(self.error_output_file_node.abspath()))
        Logs.error(self.error_output_file_node.read())

    def exec_code_generator(self, argument_string):
        """
        Execute the code generator with argument string
        :return: True on success, False on failure
        """
        command_string = '\"' + os.path.join(self.env['CODE_GENERATOR_PATH'][0],
                                             self.env['CODE_GENERATOR_EXECUTABLE']) + '\" ' + argument_string

        Logs.debug('az_code_gen: Invoking code generator with command: {}'.format(command_string))

        # Ensure not too long to execute on the current host
        host = Utils.unversioned_sys_platform()
        if (host == 'win_x64') or (host == 'win32'):
            if len(command_string) >= command_length_when_windows_fails_to_execute:
                raise Errors.WafError("az_code_gen: Unable to execute code generator due to command length being too long")

        try:
            (code_gen_output, code_gen_error_output) = self.generator.bld.cmd_and_log(
                command_string, output=BOTH, quiet=BOTH, shell=False)
            if code_gen_error_output and not code_gen_error_output.isspace():
                Logs.warn('az_code_gen: Code generator output to stderr even though it indicated success. Output was:{}\n'.format(str(code_gen_error_output)))
        except Errors.WafError as e:
            if hasattr(e, 'stdout') and e.stdout:
                self.handle_code_generator_output_errors(e.stdout)
            if hasattr(e, 'stderr') and e.stderr:
                Logs.error('az_code_gen: Utility execution produced stderr output: {}'.format(e.stderr))
            if self.error_output_file_node:
                self.print_error_output()
            return e.returncode

        self.handle_code_generator_output(code_gen_output)
        return 0

    def run_code_generator(self):
        """
        Run the code generator using at syntax for all accumulated arguments
        :return: True on success, False on failure
        """
        return self.exec_code_generator(' \"@' + self.argument_file + '\"')

    def prepare_task(self):
        # Create the directory if it doesn't already exist
        self.output_dir.mkdir()
        # We expect json output for friendlier parsing
        self.add_argument("-output-using-json")

        self.add_argument('-input-path "{}"'.format(clean_path(self.input_dir.abspath())))
        self.add_argument('-output-path "{}"'.format(clean_path(self.output_dir.abspath())))

        # Write input files to a file (command line version is too long)
        for input_file in self.inputs:
            input_file_rel_path = clean_path(input_file.path_from(self.input_dir))
            self.add_argument('-input-file "{}"'.format(input_file_rel_path))
            input_file.parent.get_bld().mkdir()

        def pypath(python_path):
            # Absolute paths are good to go as-is
            # Relative paths are assumed relative to src
            if not os.path.isabs(python_path):
                # Toss it in a node to figure out an absolute path
                python_path_node = self.generator.bld.srcnode.make_node(python_path)
                python_path = python_path_node.abspath()

            if not os.path.exists(python_path):
                Logs.warn('az_code_gen: Path given as python path does not exist: {}'.format(python_path))
            return clean_path(python_path)

        # Python paths
        self.add_argument('-python-home "{}"'.format(pypath(self.env['CODE_GENERATOR_PYTHON_HOME'])))
        for python_path in self.env['CODE_GENERATOR_PYTHON_PATHS']:
            self.add_argument('-python-path "{}"'.format(pypath(python_path)))

        # Debug python paths
        self.add_argument('-python-home-debug "{}"'.format(pypath(self.env['CODE_GENERATOR_PYTHON_HOME_DEBUG'])))
        for python_debug_path in self.env['CODE_GENERATOR_PYTHON_DEBUG_PATHS']:
            self.add_argument('-python-debug-path "{}"'.format(pypath(python_debug_path)))

        if code_generator_ignore_includes:
            self.add_argument('-ignore-includes')

        if code_generator_suppress_errors_as_warnings:
            self.add_argument('-suppress-errors-as-warnings')

        if code_generator_verbose:
            self.add_argument('-v')

        if Utils.unversioned_sys_platform().startswith('linux'):
            self.add_argument('-include-path /usr/include/c++/v1')

        for include in self.includes:
            self.add_argument('-include-path "{}"'.format(clean_path(include.abspath())))

        for include_path in self.env['CODE_GENERATOR_INCLUDE_PATHS']:
            self.add_argument('-include-path "{}"'.format(pypath(include_path)))

        if 'CODE_GENERATOR_CLANG_INCLUDE_PATH' in self.env:
            for clang_include_path in self.env['CODE_GENERATOR_CLANG_INCLUDE_PATH']:
                self.add_argument('-include-path "{}"'.format(clean_path(clang_include_path)))

        for define in self.defines:
            self.add_argument('-define {}'.format(quote(define)))

        for script_node in self.script_nodes:
            self.add_argument('-codegen-script "{}"'.format(clean_path(script_node.get_src().abspath())))

        # Include file that contains code generation tag definitions
        codegen_tags = self.env['CODE_GENERATOR_TAGS']
        if not codegen_tags:
            codegen_tags = 'Code/Framework/AzCore/AzCore/Preprocessor/CodeGen.h'
        self.add_argument('-force-include "{}"'.format(clean_path(self.generator.bld.CreateRootRelativePath(codegen_tags))))

        if self.error_output_file_node:
            self.add_argument('-redirect-output-file "{}"'.format(clean_path(self.error_output_file_node.abspath())))

        if 'CLANG_SEARCH_PATHS' in self.env:
            self.add_argument('-resource-dir "{}"'.format(self.env['CLANG_SEARCH_PATHS']['libraries'][0]))

        if 'ISYSROOT' in self.env:
            self.add_argument('-isysroot "{}"'.format(self.env['ISYSROOT']))

        if 'ANDROID' in self.defines:
            self.add_argument('-is-android-build')
            for flag in self.env['CXXFLAGS']:
                if flag.startswith('--gcc-toolchain='):
                    gcc_toolchain = flag.split('=', 2)
                    self.add_argument('-android-toolchain "{}"'.format(clean_path(gcc_toolchain[1])))
                    continue
                if flag.startswith('--target='):
                    android_target = flag.split('=', 2)
                    self.add_argument('-android-target "{}"'.format(clean_path(android_target[1])))
                    continue
                if flag.startswith('--sysroot='):
                    android_sysroot = flag.split('=', 2)
                    self.add_argument('-android-sysroot "{}"'.format(clean_path(android_sysroot[1])))
                    continue

        status, self.argument_file = self.write_argument_list_to_file()
        if not status:
            raise Errors.WafError('[ERROR] az_code_gen task creation failed')

    def run(self):
        # clear link dependencies
        self.azcg_set('link_inputs', [])
        return self.run_code_generator()

    def scan(self):
        """
        Re-use the deps from the last run, just as cxx does
        """
        dep_nodes = self.generator.bld.node_deps.get(self.uid(), [])
        return (dep_nodes, [])

    def runnable_status(self):
        """
        Ensure that all outputs exist before skipping execution.
        """
        ret = super(az_code_gen, self).runnable_status()
        if ret == Task.SKIP_ME:
            # Get output nodes from storage, check for path not signature as it may not be stable in azcg
            outputs = self.azcg_get('AZCG_OUTPUTS', [])
            for output_node in outputs:
                # If you can't find the file, running is required
                output_path = output_node.abspath()
                if not os.path.isfile(output_path):
                    Logs.debug(
                        'az_code_gen: Running task for file {}, output file {} not found.'.format(
                            self.inputs[0].abspath(), output_path))
                    return Task.RUN_ME

            # Also add the raw output path
            self.generator.env.append_unique('INCPATHS', get_azcg_output_dir_node(self.generator).abspath())

            # Also add paths we stored from prior builds
            azcg_paths = self.azcg_get('AZCG_INCPATHS', [])
            self.propagate_azcg_incpaths(azcg_paths)

            # link_inputs is a list of nodes that need to be added to the link each time
            for link_node in self.azcg_get('link_inputs', []):
                if not self.add_link_task(link_node):
                    return Task.EXCEPTION

            self.outputs = outputs
            self.generator.source += outputs
        return ret

    def get_node_from_dependency_path(self, path):
        # collect headers and add them to deps
        # this is ported from msvcdeps.py
        try:
            cached_nodes = self.bld.cached_nodes
        except:
            cached_nodes = self.bld.cached_nodes = {}

        bld = self.generator.bld
        lowercase = False
        if Utils.is_win32:
            (drive, _) = os.path.splitdrive(bld.srcnode.abspath())
            lowercase = drive == drive.lower()
        correct_case_path = bld.path.abspath()
        correct_case_path_len = len(correct_case_path)
        correct_case_path_norm = os.path.normcase(correct_case_path)

        if os.path.isabs(path):
            if Utils.is_win32:
                # Force drive letter to match conventions of main source tree
                drive, tail = os.path.splitdrive(path)

                if os.path.normcase(path[:correct_case_path_len]) == correct_case_path_norm:
                    # Path is in the sandbox, force it to be correct.  MSVC sometimes returns a lowercase path.
                    path = correct_case_path + path[correct_case_path_len:]
                else:
                    # Check the drive letter
                    if lowercase and (drive != drive.lower()):
                        path = drive.lower() + tail
                    elif (not lowercase) and (drive != drive.upper()):
                        path = drive.upper() + tail
            return path_to_node(bld.root, path, cached_nodes)
        else:
            base_node = bld.bldnode
            # when calling find_resource, make sure the path does not begin by '..'
            path = [k for k in Utils.split_path(path) if k and k != '.']
            while path[0] == '..':
                path = path[1:]
                base_node = base_node.parent

            return path_to_node(base_node, path, cached_nodes)

    def post_run(self):
        # Register output files generated by the code gen execution
        self.process_generated_output()

        bld = self.generator.bld
        dep_node = None
        resolved_nodes = []

        # Resolve registered dependencies we got into dependency nodes
        for path in self.registered_dependencies:
            dep_node = self.get_node_from_dependency_path(path)

            if dep_node:
                if not (dep_node.is_child_of(bld.srcnode) or dep_node.is_child_of(bld.bldnode)):
                    # System library
                    continue

                if dep_node in self.inputs:
                    # Self-dependency
                    continue

                if dep_node in self.outputs:
                    # Circular dependency
                    continue

                resolved_nodes.append(dep_node)
            else:
                Logs.error('az_code_gen: Unable to find dependency file as node: {}'.format(path))

        # Add azcg_deps and script nodes as dependencies
        for dep_node in itertools.chain(self.azcg_deps, self.script_nodes):
            resolved_nodes.append(dep_node)

        bld.node_deps[self.uid()] = resolved_nodes

        # force waf to recompute a full signature for this task (we may have new/deleted dependencies we need it to account for)
        try:
            del self.cache_sig
        except:
            pass

        self.azcg_set('AZCG_OUTPUTS', self.outputs)

        Task.Task.post_run(self)

        # Due to #includes of code generator header files, we can have an output node which is also an input node.
        # In addition, we are taking nodes that are not originally build nodes (e.g. header files) and building them, which alters the signature flow in Node.get_bld_sig().
        # Task.post_run() default behavior is to set the Node.sig to the task signature which will change our computed task signature because our outputs are our inputs in same cases.
        # To mitigate this, we must restore the original signature for any file that had a non-build signature previously.
        # However, we do not want to alter the signature for files that will be consumed by later tasks.
        # Therefore, we should restore signatures on any node that is not being added to the build (any output nodes not in link_task).
        for output in self.outputs:
            if not output in self.azcg_get('link_inputs', []):
                output.sig = output.cache_sig = Utils.h_file(output.abspath())

@conf
def is_azcodegen_node(ctx, node):
    azcg_node = ctx.bldnode.make_node('azcg')

    y = id(azcg_node)
    while node.parent:
        if id(node) == y:
            return True
        node = node.parent
    return False
