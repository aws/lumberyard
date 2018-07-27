#!/usr/bin/env python
# encoding: utf-8

# Modifications copyright Amazon.com, Inc. or its affiliates.

# Copyright Garmin International or its subsidiaries, 2012-2013
# Christopher Bolte: Created a copy to easier adjustment to crytek specific changes

'''
Off-load dependency scanning from Python code to MSVC compiler

This tool is safe to load in any environment; it will only activate the
MSVC exploits when it finds that a particular taskgen uses MSVC to
compile.

Empirical testing shows about a 10% execution time savings from using
this tool as compared to c_preproc.

The technique of gutting scan() and pushing the dependency calculation
down to post_run() is cribbed from gccdeps.py.
'''

import os
import sys
import tempfile
import threading

from waflib import Context, Errors, Logs, Task, Utils
from waflib.TaskGen import feature, before_method

PREPROCESSOR_FLAG = '/showIncludes'
INCLUDE_PATTERN = 'Note: including file:'

# Extensible by outside tools
supported_compilers = ['msvc']


lock = threading.Lock()

# do not call this function without holding the lock!
def _find_or_make_node_case_correct(base_node, path):
    lst = path.split('\\')

    node = base_node
    for x in lst:
        if x == '.':
            continue
        if x == '..':
            node = node.parent
            continue

        # find existing node
        try:
            node = node.children[x]
            continue
        except AttributeError:
            # node.children does not exist
            pass
        except KeyError:
            # node.children exists, but node.children[x] does not
            # find an existing node that matches name but not case
            x_lower = x.lower()
            insensitive = None
            try:
                for y in node.children:
                    if x_lower == y.lower():
                        insensitive = node.children[y]
                        break
            except RuntimeError as e:
                Logs.warn("node: {} children: {} looking for child: {}".format(node.abspath(), node.children, x_lower))
                # forward error
                raise

            # found a node, use it
            if insensitive:
                node = insensitive
                continue

            # fallthrough, create a new node

        # create a node, using the case on disk for the node name
        found_node = None
        x_lower = x.lower()
        for item in os.listdir(node.abspath()):
            # find a directory entry on disk from the parent that matches caseless
            if x_lower == item.lower():
                found_node = node.make_node(item)
                break
        node = found_node

        # this should always find a node, the compile task just succeeded, but..
        if not node:
            # this can occur if node.children has 2 entries that differ only by case
            raise ValueError('could not find %r for %r' % (path, base_node))

    return node


@feature('c', 'cxx')
@before_method('process_source')
def apply_msvcdeps_flags(taskgen):
    if taskgen.env.CC_NAME not in supported_compilers:
        return

    # Figure out what casing conventions the user's shell used when
    # launching Waf
    (drive, _) = os.path.splitdrive(taskgen.bld.srcnode.abspath())
    taskgen.msvcdeps_drive_lowercase = drive == drive.lower()
    
    # Don't append show includes when in special single file mode
    if taskgen.bld.is_option_true('show_preprocessed_file'):
        return
    if taskgen.bld.is_option_true('show_disassembly'):
        return  
    
    for flag in ('CFLAGS', 'CXXFLAGS'):
        if taskgen.env.get_flat(flag).find(PREPROCESSOR_FLAG) < 0:
            taskgen.env.append_value(flag, PREPROCESSOR_FLAG)


'''
Register a task subclass that has hooks for running our custom
dependency calculations rather than the C/C++ stock c_preproc
method.
'''
def wrap_compiled_task(classname):
    derived_class = type(classname, (Task.classes[classname],), {})

    def post_run(self):        
        if self.env.CC_NAME not in supported_compilers:
            return super(derived_class, self).post_run()

        if getattr(self, 'cached', None):
            return Task.Task.post_run(self)

        bld = self.generator.bld
        unresolved_names = []
        resolved_nodes = []

        if (len(self.src_deps_paths) + len(self.bld_deps_paths)):
            # The following code is executed by threads, it is not safe, so a lock is needed...
            with lock:
                for path in self.src_deps_paths:
                    node = _find_or_make_node_case_correct(bld.srcnode, path)
                    resolved_nodes.append(node)
                for path in self.bld_deps_paths:
                    node = _find_or_make_node_case_correct(bld.bldnode, path)
                    resolved_nodes.append(node)


        # Bug workaround.  With VS2015, compiling a pch with /showIncludes with a pre-existing .pch/.obj that doesn't
        # need updates, results in no include file output.  If we save that empty list, subsequent builds will fail to
        # rebuild properly.  Only update the save dependencies for pch files if output was detected; otherwise use the
        # existing dependency set.  This doesn't affect VS2017.
        if 0 == len(resolved_nodes) and self.__class__.__name__ == 'pch_msvc':
            Logs.warn("no dependencies returned for {}, using last known dependencies instead".format(str(self)))
        else:
            bld.node_deps[self.uid()] = resolved_nodes
            bld.raw_deps[self.uid()] = unresolved_names

        # Free memory (200KB for each file in CryEngine, without UberFiles, this accumulates to 1 GB)
        del self.src_deps_paths
        del self.bld_deps_paths

        try:
            del self.cache_sig
        except:
            pass

        Task.Task.post_run(self)

    def scan(self):
        if self.env.CC_NAME not in supported_compilers:
            return super(derived_class, self).scan()

        resolved_nodes = self.generator.bld.node_deps.get(self.uid(), [])
        unresolved_names = []
        return (resolved_nodes, unresolved_names)

    def exec_response_command(self, cmd, **kw):
        # exec_response_command() is only called from inside msvc.py anyway
        assert self.env.CC_NAME in supported_compilers
        
        # Only bother adding '/showIncludes' to compile tasks
        #if isinstance(self, (c.c, cxx.cxx, msvcdeps.pch_msvc)):
        if True:                
            try:
                # The Visual Studio IDE adds an environment variable that causes
                # the MS compiler to send its textual output directly to the
                # debugging window rather than normal stdout/stderr.
                #
                # This is unrecoverably bad for this tool because it will cause
                # all the dependency scanning to see an empty stdout stream and
                # assume that the file being compiled uses no headers.
                #
                # See http://blogs.msdn.com/b/freik/archive/2006/04/05/569025.aspx
                #
                # Attempting to repair the situation by deleting the offending
                # envvar at this point in tool execution will not be good enough--
                # its presence poisons the 'waf configure' step earlier. We just
                # want to put a sanity check here in order to help developers
                # quickly diagnose the issue if an otherwise-good Waf tree
                # is then executed inside the MSVS IDE.
                                # Note seems something changed, and this env var cannot be found anymore
                #assert 'VS_UNICODE_OUTPUT' not in kw['env']

                # Note to future explorers: sometimes msvc doesn't return any includes even though
                # /showIncludes was specified.  The only repro I've found is building pch files,
                # when the .pch and .obj file did not actually change (their dependencies didn't change).
                # This will then clear out the dependencies of that file, and may have future issues triggering rebuild.
                # The not-so-good workaround is to delete the pch before rebuild

                tmp = None

                # This block duplicated from Waflib's msvc.py
                if sys.platform.startswith('win') and isinstance(cmd, list) and len(' '.join(cmd)) >= 16384:
                    tmp_files_folder = self.generator.bld.get_bintemp_folder_node().make_node('TempFiles')
                    program = cmd[0]
                    cmd = [self.quote_response_command(x) for x in cmd]
                    (fd, tmp) = tempfile.mkstemp(dir=tmp_files_folder.abspath())
                    os.write(fd, '\r\n'.join(cmd[1:]).encode())
                    os.close(fd)
                    cmd = [program, '@' + tmp]
                # ... end duplication

                self.src_deps_paths = set()
                self.bld_deps_paths = set()

                kw['env'] = kw.get('env', os.environ.copy())
                kw['cwd'] = kw.get('cwd', os.getcwd())
                kw['quiet'] = Context.STDOUT
                kw['output'] = Context.STDOUT

                out = []
                try:                    
                    raw_out = self.generator.bld.cmd_and_log(cmd, **kw)
                    ret = 0
                except Errors.WafError as e:
                    try:
                        # Get error output if failed compilation                                        
                        raw_out = e.stdout
                        ret = e.returncode
                    except:                                     
                        # Fallback (eg the compiler itself is not found)                                        
                        raw_out = str(e)
                        ret = -1

                bld = self.generator.bld
                srcnode_abspath_lower = bld.srcnode.abspath().lower()
                bldnode_abspath_lower = bld.bldnode.abspath().lower()
                srcnode_abspath_lower_len = len(srcnode_abspath_lower)
                bldnode_abspath_lower_len = len(bldnode_abspath_lower)

                show_includes = bld.is_option_true('show_includes')
                for line in raw_out.splitlines():
                    if line.startswith(INCLUDE_PATTERN):
                        if show_includes:
                            out.append(line)
                        inc_path = line[len(INCLUDE_PATTERN):].strip()
                        if Logs.verbose:
                            Logs.debug('msvcdeps: Regex matched %s' % inc_path)
                        # normcase will change '/' to '\\' and lowercase everything, use sparingly
                        # normpath is safer, but we really want to ignore all permutations of these roots
                        norm_path = os.path.normpath(inc_path)
                        # The bld node may be embedded under the source node, eg dev/BinTemp/flavor.
                        # check if the path is in the src node first, and bld node second, allowing for this overlap
                        if norm_path[:srcnode_abspath_lower_len].lower() == srcnode_abspath_lower:
                            subpath = norm_path[srcnode_abspath_lower_len + 1:]
                            self.src_deps_paths.add(subpath)
                            continue
                        if norm_path[:bldnode_abspath_lower_len].lower() == bldnode_abspath_lower:
                            subpath = norm_path[bldnode_abspath_lower_len + 1:]
                            self.bld_deps_paths.add(subpath)
                            continue
                        # System library
                        if Logs.verbose:
                            Logs.debug('msvcdeps: Ignoring system include %r' % inc_path)
                    else:
                        out.append(line)

                # Pipe through the remaining stdout content (not related to /showIncludes)
                show_output = False
                for i in out:
                    if ' ' in i:
                        show_output = True
                        break
                # Dont show outputs immediately when we have an error, defer those till we have better information
                if show_output:
                    if self.generator.bld.logger:
                        self.generator.bld.logger.debug('out: %s' % '\n'.join(out))
                    else:                                       
                        sys.stdout.write(os.linesep.join(out) + '\n')

            finally:
                if tmp:
                    try:
                        os.remove(tmp)
                    except OSError:
                        pass
            
                        # Create custom error message for improved readibility
            if ret != 0:
                self.err_msg = '<++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++>\n' 
                self.err_msg += "Compilation failed - File: %r, Module: %r, Configuration: '%s|%s', error code %d\n" % (os.path.basename(self.outputs[0].abspath()), self.generator.target, self.generator.bld.env['PLATFORM'], self.generator.bld.env['CONFIGURATION'], ret )
                self.err_msg += "\tInput Files:   '%s'\n" % ', '.join(i.abspath() for i in self.inputs)
                self.err_msg += "\tOutput Files:  '%s'\n" % (', ').join(i.abspath() for i in self.outputs)
                self.err_msg += "\tCommand:       '%s'\n" % ' '.join(self.last_cmd)
                out_merged = ''
                for line in out:
                    out_merged += '\t\t' + line + '\n'
                self.err_msg += "\tOutput:\n%s" % out_merged
                self.err_msg += "<++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++>\n"

            return ret
        else:
            # Use base class's version of this method for linker tasks
            return super(derived_class, self).exec_response_command(cmd, **kw)

    def can_retrieve_cache(self):        
        # msvcdeps and netcaching are incompatible, so disable the cache
        if self.env.CC_NAME not in supported_compilers:
            return super(derived_class, self).can_retrieve_cache()
        self.nocache = True # Disable sending the file to the cache
        return False

    derived_class.post_run = post_run
    derived_class.scan = scan
    derived_class.exec_response_command = exec_response_command
    derived_class.can_retrieve_cache = can_retrieve_cache


for compile_task in ('c', 'cxx','pch_msvc'):
    wrap_compiled_task(compile_task)


'''
This module adds nodes during the parallel Build.compile() step
Other modules are also doing this, which requires a some thread safety
Register a module hook and a task subclass that has hooks for running our custom
scan method that locks the node access to one thread to reduce threading issues
'''
def sync_lookup_deps(root, deps_paths):
    nodes = []
    names = []

    with lock:
        for x in deps_paths:
            nd = root.find_resource(x)
            if nd:
                nodes.append(nd)
            else:
                names.append(x)

    return (nodes, names)


def sync_node_access_during_compile(classname):
    derived_class = type(classname, (Task.classes[classname],), {})

    def post_run(self):
        with lock:
            rv = super(derived_class, self).post_run()
        return rv

    derived_class.post_run = post_run


for compile_task in ('winrc', ):
    sync_node_access_during_compile(compile_task)

