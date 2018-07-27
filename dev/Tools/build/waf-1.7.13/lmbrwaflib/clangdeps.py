#!/usr/bin/env python
# encoding: utf-8

# Modifications copyright Amazon.com, Inc. or its affiliates.

# derived from msvcdeps.py, which was derived from gccdeps.py
# gccdeps uses -MD which exports dependencies to a standalone file
# this module scrapes the dependencies from stderr using -H


import os, sys, tempfile, threading, re

from waflib import Context, Errors, Logs, Task, Utils
from waflib.Tools import c_preproc, c, cxx
from waflib.TaskGen import feature, after, after_method, before_method
import waflib.Node
import subprocess

from cry_utils import get_command_line_limit

# clang dependencies are a set of dots indicating include depth, followed by the absolute path
#   ex:  ... /dev/Code/Framework/AzCore/AzCore/base.h
re_clang_include = re.compile(r'\.\.* (.*)$')

supported_compilers = ['clang', 'clang++',
]

lock = threading.Lock()
nodes = {}  # Cache the path -> Node lookup


#############################################################################
#############################################################################
# Special handling to be able to create response files
# Based in msvc from WAF
def quote_response_command_clang(self, flag):
    flag = flag.replace('\\', '\\\\')  # escape any backslashes
    flag = '"%s"' % flag
    return flag


#############################################################################
# use a command file to invoke the command if the path is longer than maxpath
def exec_response_command_clang(self, cmd, **kw):
    try:
        tmp = None
        arg_max = get_command_line_limit()

        # 1) Join options that carry no space are joined e.g. /Fo FilePath -> /FoFilePath
        # 2) Join options that carry a ':' as last character : e.g. /OUT: FilePath -> /OUT:FilePath
        if isinstance(cmd, list):
            lst = []
            carry = ''
            join_with_next_list_item = ['/Fo', '/doc', '/Fi', '/Fa']
            for a in cmd:
                if a in join_with_next_list_item or a[-1] == ':':
                    carry = a
                else:
                    lst.append(carry + a)
                    carry = ''

            cmd = lst

        if isinstance(cmd, list) and len(' '.join(cmd)) >= arg_max:
            program = cmd[0]  # unquoted program name, otherwise exec_command will fail
            if program == 'ar' and sys.platform.startswith('darwin'):
                Logs.error('ar does not support response files on osx')
            else:
                cmd = [self.quote_response_command(x) for x in cmd]
                (fd, tmp) = tempfile.mkstemp(prefix=self.outputs[0].name)
                os.write(fd, '\n'.join(cmd[1:]).encode())
                os.close(fd)
                cmd = [program, '@' + tmp]
        # no return here, that's on purpose
        ret = self.generator.bld.exec_command(cmd, **kw)
    finally:
        if tmp:
            try:
                os.remove(tmp)
            except OSError:
                pass  # anti-virus and indexers can keep the files open -_-
    return ret


#############################################################################
def exec_command_clang(self, *k, **kw):
    bld = self.generator.bld
    try:
        if not kw.get('cwd', None):
            kw['cwd'] = bld.cwd
    except AttributeError:
        bld.cwd = kw['cwd'] = bld.variant_dir

    return exec_response_command_clang(self, k[0], **kw)


#############################################################################
def wrap_class_clang(class_name):
    """
    @response file workaround for command-line length limits
    The indicated task class is replaced by a subclass to prevent conflicts in case the class is wrapped more than once
    """
    cls = Task.classes.get(class_name, None)

    if not cls:
        return None

    derived_class = type(class_name, (cls,), {})

    def exec_command(self, *k, **kw):
        if self.env.CC_NAME in supported_compilers:
            return exec_command_clang(self, *k, **kw)
        elif self.env.CC_NAME in ['gcc']:
            # workaround: the previous module was intercepting commands to gcc and clang
            # and issuing them with response files.  This new module only handled clang, and this
            # this is the only place that needs response files
            return exec_command_clang(self, *k, **kw)
        else:
            return super(derived_class, self).exec_command(*k, **kw)

    def exec_response_command(self, cmd, **kw):
        if self.env.CC_NAME in supported_compilers:
            return exec_response_command_clang(self, cmd, **kw)
        else:
            return super(derived_class, self).exec_response_command(cmd, **kw)

    def quote_response_command(self, *k, **kw):
        if self.env.CC_NAME in supported_compilers:
            return quote_response_command_clang(self, *k, **kw)
        else:
            return super(derived_class, self).quote_response_command(*k, **kw)


    # Chain-up monkeypatch needed since these commands are in base class API
    derived_class.exec_command = exec_command
    derived_class.exec_response_command = exec_response_command
    derived_class.quote_response_command = quote_response_command

    return derived_class


#############################################################################
## Wrap call exec commands
for k in 'c cxx pch_clang cprogram cxxprogram cshlib cxxshlib cstlib cxxstlib'.split():
    wrap_class_clang(k)


#############################################################################
## Task to create pch files
class pch_clang(waflib.Task.Task):
    run_str = '${CXX} -x c++-header ${ARCH_ST:ARCH} ${CXXFLAGS} ${CPPFLAGS} ${FRAMEWORKPATH_ST:FRAMEWORKPATH} ${CPPPATH_ST:INCPATHS} ${DEFINES_ST:DEFINES} ${SRC} -o ${TGT}'
    scan = c_preproc.scan
    color = 'BLUE'

    # bindings to our executor, allows us to filter the dependencies from the output

    def exec_command(self, *k, **kw):
        return exec_command_clang(self, *k, **kw)

    def exec_response_command(self, *k, **kw):
        return exec_response_command_clang(self, *k, **kw)

    def quote_response_command(self, *k, **kw):
        return quote_response_command_clang(self, *k, **kw)


#############################################################################
## add precompiled header to c++ compile commandline, creates and add pch tasks if needed
@feature('cxx')
@after_method('apply_incpaths')
def add_pch_clang(self):
    if self.env.CC_NAME not in supported_compilers:
        return

    # is there a pch specified?
    if not getattr(self, 'pch', ''):
        return

    # is the pch disabled?
    if not self.bld.is_option_true('use_precompiled_header'):
        return

    # skip pch generation if objective-c is being used
    if 'objective-c++' in self.env.CXXFLAGS:
        return

    # Always assume only one PCH File
    pch_source = self.to_nodes(self.pch)[0]

    pch_header = pch_source.change_ext('.h')
    # Generate PCH per target project idx
    # Avoids the case where two project have the same PCH output path but compile the PCH with different compiler options i.e. defines, includes, ...
    pch_file = pch_source.change_ext('.%d.h.pch' % self.target_uid)

    # Create PCH Task
    pch_task = self.create_task('pch_clang', pch_source, pch_file)

    # we need to get the absolute path to the pch.h.pch
    # which we then need to include as pch.h
    # Since WAF is so smart to generate the source path for the header
    # we do the name replacement outself :)
    pch_file_name = pch_file.abspath()
    pch_file_name = pch_file_name.replace('.h.pch', '.h')

    # Append PCH File to each compile task
    for t in getattr(self, 'compiled_tasks', []):
        if t.__class__.__name__ in ['cxx', 'qxx']:  # pch is disabled in compile flags, so skip those tasks
            # don't apply precompiled headers to objective c++ files.  The module may still use pch if
            # the compile task isn't using -x objective-c++
            if t.inputs[0].name.endswith('.mm'):
                continue

            # add the pch file as a include on the command line
            pch_flag = '-include' + pch_file_name
            t.env.append_value('CXXFLAGS', pch_flag)
            t.env.append_value('CFLAGS', pch_flag)

            # if rtti is enabled for this source file then we need to make sure
            # rtti is enabled in the pch otherwise clang will fail to compile
            if ('-fno-rtti' not in t.env['CXXFLAGS']):
                pch_task.env['CXXFLAGS'] = list(filter(lambda r:not r.startswith('-fno-rtti'), pch_task.env['CXXFLAGS']))

            # Append PCH to task input to ensure correct ordering.  The task won't proceed until this is generated
            t.dep_nodes.append(pch_file)

            # save the pch file name, it must be manually added to the dependencies because its force
            # included on the command line
            t.pch_file_name = pch_file.abspath()


#############################################################################
## add dependency flag to compile to generate dependencies
@feature('c', 'cxx')
@before_method('process_source')
def add_clangdeps_flags(taskgen):
    if taskgen.env.CC_NAME not in supported_compilers:
        return

    # Don't append show includes when in special single file mode
    if taskgen.bld.is_option_true('show_preprocessed_file'):
        return
    if taskgen.bld.is_option_true('show_disassembly'):
        return

    # add flag to tell compiler to output dependencies
    for flag in ('CFLAGS', 'CXXFLAGS'):
        # -H uses stderr for displaying dependencies.  -MD and -MDD will output to a .d file
        if taskgen.env.get_flat(flag).find('-H') < 0:
            taskgen.env.append_value(flag, '-H')


#############################################################################
## convert path to node.  Dependencies are returned from clang as paths, but we want to know which nodes need to be rebuilt
def path_to_node(base_node, path, cached_nodes, b_drive_hack):
    # Take the base node and the path and return a node
    # Results are cached because searching the node tree is expensive
    # The following code is executed by threads, it is not safe, so a lock is needed...
    if getattr(path, '__hash__'):
        node_lookup_key = (base_node, path)
    else:
        # Not hashable, assume it is a list and join into a string
        node_lookup_key = (base_node, os.path.sep.join(path))
    try:
        lock.acquire()
        node = cached_nodes[node_lookup_key]
    except KeyError:
        node = base_node.find_resource(path)
        if not node and b_drive_hack: # To handle absolute path on C when building on another drive
            node = base_node.make_node(path)
        cached_nodes[node_lookup_key] = node
    finally:
        lock.release()
    return node


#############################################################################
## dependency handler
def wrap_compiled_task_clang(classname):
    derived_class = type(classname, (waflib.Task.classes[classname],), {})

    def post_run(self):
        if self.env.CC_NAME not in supported_compilers:
            return super(derived_class, self).post_run()

        if getattr(self, 'cached', None):
            return Task.Task.post_run(self)

        bld = self.generator.bld
        resolved_nodes = []

        # Dynamically bind to the cache
        try:
            cached_nodes = bld.cached_nodes
        except AttributeError:
            cached_nodes = bld.cached_nodes = {}

        # convert paths to nodes
        for path in self.clangdeps_paths:
            node = None
            assert os.path.isabs(path)

            drive_hack = False
            node = path_to_node(bld.root, path, cached_nodes, drive_hack)

            if not node:
                raise ValueError('could not find %r for %r' % (path, self))
            else:
                if not c_preproc.go_absolute:
                    if not (node.is_child_of(bld.srcnode) or node.is_child_of(bld.bldnode)):
                        # System library
                        Logs.debug('clangdeps: Ignoring system include %r' % node)
                        continue

                if id(node) == id(self.inputs[0]):
                    # Self-dependency
                    continue

                resolved_nodes.append(node)

        bld.node_deps[self.uid()] = resolved_nodes
        bld.raw_deps[self.uid()] = []

        # Free memory (200KB for each file in CryEngine, without UberFiles, this accumulates to 1 GB)
        del self.clangdeps_paths

        # force recompute task signature
        try:
            del self.cache_sig
        except:
            pass

        waflib.Task.Task.post_run(self)

    def scan(self):
        # no previous signature or dependencies have changed for this node
        if self.env.CC_NAME not in supported_compilers:
            return super(derived_class, self).scan()

        resolved_nodes = self.generator.bld.node_deps.get(self.uid(), [])
        unresolved_names = []
        return (resolved_nodes, unresolved_names)

    def exec_command(self, cmd, **kw):
        if self.env.CC_NAME not in supported_compilers:
            return super(derived_class, self).exec_command(cmd, **kw)

        try:

            tmp = None

            self.clangdeps_paths = set()

            if getattr(self, 'pch_file_name', None):
                self.clangdeps_paths.add(self.pch_file_name);

            kw['env'] = kw.get('env', os.environ.copy())
            kw['cwd'] = kw.get('cwd', os.getcwd())
            kw['quiet'] = Context.BOTH
            kw['output'] = Context.BOTH

            out = []
            raw_err = ''
            raw_out = ''
            try:
                (raw_out, raw_err) = self.generator.bld.cmd_and_log(cmd, **kw)
                ret = 0
            except Errors.WafError as e:
                try:
                    # Get error output if failed compilation
                    raw_out = e.stdout
                    raw_err = e.stderr
                    ret = e.returncode
                except:
                    # Fallback (eg the compiler itself is not found)
                    raw_out = str(e)
                    ret = -1

            # error output is intermixed with include paths.  Use regex to filter them out
            for line in raw_err.splitlines():
                res = re.match(re_clang_include, line)
                if res != None:
                    inc_path = res.group(1)
                    Logs.debug('clangdeps: Regex matched %s' % inc_path)
                    if not os.path.isabs(inc_path):
                        inc_path = os.path.join(self.generator.bld.path.abspath(), inc_path)
                    self.clangdeps_paths.add(inc_path)
                    if self.generator.bld.is_option_true('show_includes'):
                        out.append(line)
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
            self.err_msg += "Compilation failed - File: %r, Module: %r, Configuration: '%s|%s', error code %d\n" % (
                os.path.basename(self.outputs[0].abspath()), self.generator.target,
                self.generator.bld.env['PLATFORM'],
                self.generator.bld.env['CONFIGURATION'], ret)
            self.err_msg += "\tInput Files:   '%s'\n" % ', '.join(i.abspath() for i in self.inputs)
            self.err_msg += "\tOutput Files:  '%s'\n" % (', ').join(i.abspath() for i in self.outputs)
            self.err_msg += "\tCommand:       '%s'\n" % ' '.join(self.last_cmd)
            out_merged = ''
            for line in out:
                out_merged += '\t\t' + line + '\n'
            self.err_msg += "\tOutput:\n%s" % out_merged
            self.err_msg += "<++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++>\n"

        return ret

    def can_retrieve_cache(self):
        # msvcdeps and netcaching are incompatible, so disable the cache
        if self.env.CC_NAME not in supported_compilers:
            return super(derived_class, self).can_retrieve_cache()
        self.nocache = True  # Disable sending the file to the cache
        return False

    derived_class.post_run = post_run
    derived_class.scan = scan
    derived_class.exec_command = exec_command
    derived_class.can_retrieve_cache = can_retrieve_cache


#############################################################################
## Wrap compile commands to track dependencies for these types of files
for compile_task in ('c', 'cxx', 'pch_clang'):
    wrap_compiled_task_clang(compile_task)
