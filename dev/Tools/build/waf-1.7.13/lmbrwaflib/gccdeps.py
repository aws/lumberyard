#!/usr/bin/env python
# encoding: utf-8

# Modifications copyright Amazon.com, Inc. or its affiliates.

# Thomas Nagy, 2008-2010 (ita)
"""
Execute the tasks with gcc -MD, read the dependencies from the .d file
and prepare the dependency calculation for the next run
"""

import os, re, threading
from waflib import Task, Logs, Utils, Errors
from waflib.Tools import c_preproc
from waflib.TaskGen import before_method, feature

lock = threading.Lock()

preprocessor_flag = '-MD'

# Third-party tools are allowed to add extra names in here with append()
supported_compilers = ['gcc', 'icc']

@feature('c')
@before_method('process_source')
def add_mmd_cc(self):
    if self.env.CC_NAME in supported_compilers and self.env.get_flat('CFLAGS').find(preprocessor_flag) < 0:
        self.env.append_value('CFLAGS', [preprocessor_flag])

@feature('cxx')
@before_method('process_source')
def add_mmd_cxx(self):
    if self.env.CC_NAME in supported_compilers and self.env.get_flat('CXXFLAGS').find(preprocessor_flag) < 0:
        self.env.append_value('CXXFLAGS', [preprocessor_flag])

def scan(self):
    "the scanner does not do anything initially"
    if self.env.CC_NAME not in supported_compilers:
        return self.no_gccdeps_scan()
    nodes = self.generator.bld.node_deps.get(self.uid(), [])
    names = []
    return (nodes, names)

re_o = re.compile("\.o$")
re_splitter = re.compile(r'(?<!\\)\s+') # split by space, except when spaces are escaped

def remove_makefile_rule_lhs(line):
    # Splitting on a plain colon would accidentally match inside a
    # Windows absolute-path filename, so we must search for a colon
    # followed by whitespace to find the divider between LHS and RHS
    # of the Makefile rule.
    rulesep = ': '

    sep_idx = line.find(rulesep)
    if sep_idx >= 0:
        return line[sep_idx + 2:]
    else:
        return line

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

def post_run(self):
    # The following code is executed by threads, it is not safe, so a lock is needed...

    if self.env.CC_NAME not in supported_compilers:
        return self.no_gccdeps_post_run()

    if getattr(self, 'cached', None):
        return Task.Task.post_run(self)

    # Do not check dependencies for disassembly and preprocessed files as this is already the final output
    bld = self.generator.bld
    if bld.is_option_true('show_preprocessed_file') or bld.is_option_true('show_disassembly'):
        return Task.Task.post_run(self)

    name = self.outputs[0].abspath()
    name = re_o.sub('.d', name)
    txt = Utils.readf(name)
    #os.remove(name)

    # Compilers have the choice to either output the file's dependencies
    # as one large Makefile rule:
    #
    #   /path/to/file.o: /path/to/dep1.h \
    #                    /path/to/dep2.h \
    #                    /path/to/dep3.h \
    #                    ...
    #
    # or as many individual rules:
    #
    #   /path/to/file.o: /path/to/dep1.h
    #   /path/to/file.o: /path/to/dep2.h
    #   /path/to/file.o: /path/to/dep3.h
    #   ...
    #
    # So the first step is to sanitize the input by stripping out the left-
    # hand side of all these lines. After that, whatever remains are the
    # implicit dependencies of task.outputs[0]
    txt = '\n'.join([remove_makefile_rule_lhs(line) for line in txt.splitlines()])

    # Now join all the lines together
    txt = txt.replace('\\\n', '')

    val = txt.strip()
    lst = val.split(':')
    val = [x.replace('\\ ', ' ') for x in re_splitter.split(val) if x]

    nodes = []

    # Dynamically bind to the cache
    try:
        cached_nodes = bld.cached_nodes
    except AttributeError:
        cached_nodes = bld.cached_nodes = {}

    for x in val:
        node = None
        # Remove leading and tailing double quotes
        if x[0] == '"':
            x = x[1:]
        if x[len(x)-1] == '"':
            x = x[:len(x)-1]

        drive_hack = False
        if os.path.isabs(x):
            # HACK: for reasons unknown, some of the android library includes have a ':' appended to the path
            # causing the following to fail
            if 'android' in self.env['PLATFORM']:
                if x[-1] == ':':
                    x = x[:-1]

            node = path_to_node(bld.root, x, cached_nodes, drive_hack)
        else:
            path = bld.bldnode
            # when calling find_resource, make sure the path does not begin by '..'
            x = [k for k in Utils.split_path(x) if k and k != '.']
            while lst and x[0] == '..':
                x = x[1:]
                path = path.parent
            node = path_to_node(path, x, cached_nodes, drive_hack)

        if not node:
            raise ValueError('could not find %r for %r' % (x, self))
        else:
            if not c_preproc.go_absolute:
                if not (node.is_child_of(bld.srcnode) or node.is_child_of(bld.bldnode)):
                    continue

            if id(node) == id(self.inputs[0]):
                # ignore the source file, it is already in the dependencies
                # this way, successful config tests may be retrieved from the cache
                continue

            if node in self.outputs:
                # Circular dependency
                continue

            nodes.append(node)

    Logs.debug('deps: real scanner for %s returned %s' % (str(self), str(nodes)))

    bld.node_deps[self.uid()] = nodes
    bld.raw_deps[self.uid()] = []

    try:
        del self.cache_sig
    except:
        pass

    Task.Task.post_run(self)

for name in 'c cxx'.split():
    try:
        cls = Task.classes[name]
    except KeyError:
        pass
    else:
        cls.no_gccdeps_post_run = cls.post_run
        cls.no_gccdeps_scan = cls.scan

        cls.post_run = post_run
        cls.scan = scan

