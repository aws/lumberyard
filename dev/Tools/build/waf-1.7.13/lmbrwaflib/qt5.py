#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2006-2015 (ita)

# Modifications copyright Amazon.com, Inc. or its affiliates.

"""

Tool Description
================

This tool helps with finding Qt5 tools and libraries,
and also provides syntactic sugar for using Qt5 tools.

The following snippet illustrates the tool usage::

    def options(opt):
        opt.load('compiler_cxx qt5')

    def configure(conf):
        conf.load('compiler_cxx qt5')

    def build(bld):
        bld(
            features = ['qt5','cxx','cxxprogram'],
            uselib   = ['QTCORE','QTGUI','QTOPENGL','QTSVG'],
            source   = 'main.cpp textures.qrc aboutDialog.ui',
            target   = 'window',
        )

Here, the UI description and resource files will be processed
to generate code.

Usage
=====

Load the "qt5" tool.

You also need to edit your sources accordingly:

- the normal way of doing things is to have your C++ files
  include the .moc file.
  This is regarded as the best practice (and provides much faster
  compilations).
  It also implies that the include paths have beenset properly.

- to have the include paths added automatically, use the following::

     from waflib.TaskGen import feature, before_method, after_method
     @feature('cxx')
     @after_method('process_source')
     @before_method('apply_incpaths')
     def add_includes_paths(self):
        incs = set(self.to_list(getattr(self, 'includes', '')))
        for x in self.compiled_tasks:
            incs.add(x.inputs[0].parent.path_from(self.path))
        self.includes = list(incs)

Note: another tool provides Qt processing that does not require
.moc includes, see 'playground/slow_qt/'.

A few options (--qt{dir,bin,...}) and environment variables
(QT5_{ROOT,DIR,MOC,UIC,XCOMPILE}) allow finer tuning of the tool,
tool path selection, etc; please read the source for more info.

"""

try:
    from xml.sax import make_parser
    from xml.sax.handler import ContentHandler
except ImportError:
    has_xml = False
    ContentHandler = object
else:
    has_xml = True

import os, sys, re, time, shutil, stat
from waflib.Tools import cxx
from waflib import Task, Utils, Options, Errors, Context
from waflib.TaskGen import feature, after_method, extension, before_method
from waflib.Configure import conf
from waflib.Tools import c_preproc
from waflib import Logs
from generate_uber_files import UBER_HEADER_COMMENT
import copy_tasks
from collections import defaultdict
from threading import Lock
from sets import Set
import lmbr_setup_tools

MOC_H = ['.h', '.hpp', '.hxx', '.hh']
"""
File extensions associated to the .moc files
"""

EXT_RCC = ['.qrc']
"""
File extension for the resource (.qrc) files
"""

EXT_UI  = ['.ui']
"""
File extension for the user interface (.ui) files
"""

EXT_QT5 = ['.cpp', '.cc', '.cxx', '.C', '.mm']
"""
File extensions of C++ files that may require a .moc processing
"""

QT5_LIBS = '''
qtmain
Qt5Bluetooth
Qt5CLucene
Qt5Concurrent
Qt5Core
Qt5DBus
Qt5Declarative
Qt5DesignerComponents
Qt5Designer
Qt5Gui
Qt5Help
Qt5MacExtras
Qt5MultimediaQuick_p
Qt5Multimedia
Qt5MultimediaWidgets
Qt5Network
Qt5Nfc
Qt5OpenGL
Qt5Positioning
Qt5PrintSupport
Qt5Qml
Qt5QuickParticles
Qt5Quick
Qt5QuickTest
Qt5Script
Qt5ScriptTools
Qt5Sensors
Qt5SerialPort
Qt5Sql
Qt5Svg
Qt5Test
Qt5WebKit
Qt5WebKitWidgets
Qt5WebChannel
Qt5Widgets
Qt5WinExtras
Qt5X11Extras
Qt5XmlPatterns
Qt5Xml'''


# Search pattern to find the required #include <*.moc> lines in the source code to identify the header files that need
# moc'ing.  The path of the moc file must be relative to the current project root
INCLUDE_MOC_RE = re.compile(r'\s*\#include\s+[\"<](.*.moc)[\">]',flags=re.MULTILINE)
INCLUDE_SRC_RE = re.compile(r'\s*\#include\s+[\"<](.*.(cpp|cxx|cc))[\">]',flags=re.MULTILINE)
QOBJECT_RE = re.compile(r'\s*Q_OBJECT\s*', flags=re.MULTILINE)


# Derive a specific moc_files.<idx> folder name based on the base bldnode and idx
def get_target_qt5_root(ctx, target_name, idx):

    base_qt_node = ctx.bldnode.make_node('qt5/{}.{}'.format(target_name,idx))
    return base_qt_node

# Change a target node from a changed extension to one marked as QT code generated
# The qt5 generated files are restricted to the build folder.  That means
# each project cannot use any QT generated artifacts that do no exist within its project boundaries.
def change_target_qt5_node(ctx, project_path, target_name, relpath_target, idx):
    relpath_project = project_path.relpath()

    if relpath_target.startswith(relpath_project):
        # Strip out the project relative path and use that as the target_qt5 relative path
        restricted_path = relpath_target.replace(relpath_project,'')
    elif relpath_target.startswith('..'):
        # Special case.  If the target and project rel paths dont align, then the target node is outside of the
        # project folder.  (ie there is a qt-related file in the waf_files that is outside the project's context path)

        # If the file is an include or moc file, it must reside inside the project context, because it will be
        # included based on an expected project relative path
        target_node_name_lower = relpath_target.lower()
        if target_node_name_lower.endswith(".moc") or target_node_name_lower.endswith(".h"):
            ctx.fatal("QT target {} for project {} cannot exist outside of its source folder context.".format(relpath_target, target_name))

        restricted_path = "__/{}.{}/{}".format(target_name, idx, target_name)
    else:
        restricted_path = relpath_target

    target_node_subdir = os.path.dirname(restricted_path)

    # Change the output target to the specific moc file folder
    output_qt_dir = get_target_qt5_root(ctx, target_name,idx).make_node(target_node_subdir)
    output_qt_dir.mkdir()

    output_qt_node = output_qt_dir.make_node(os.path.split(relpath_target)[1])
    return output_qt_node


class qxx(Task.classes['cxx']):
    """
    Each C++ file can have zero or several .moc files to create.
    They are known only when the files are scanned (preprocessor)
    To avoid scanning the c++ files each time (parsing C/C++), the results
    are retrieved from the task cache (bld.node_deps/bld.raw_deps).
    The moc tasks are also created *dynamically* during the build.
    """

    def __init__(self, *k, **kw):
        Task.Task.__init__(self, *k, **kw)

        if 'qt5' in self.generator.features and self.env.QMAKE:
            # If QT5 is enabled, then signal that moc scanning is needed
            self.moc_done = 0
        else:
            # Otherwise, signal that moc scanning can be skipped
            self.moc_done = 1

        self.dep_moc_files = {}

    def __str__(self):
        "string to display to the user"
        env = self.env
        src_str = ' '.join([a.nice_path() for a in self.inputs])
        tgt_str = ' '.join([a.nice_path() for a in self.outputs])
        if self.outputs and self.inputs:
            sep = ' -> '
        else:
            sep = ''
        name = self.__class__.__name__.replace('_task', '') + ' (' + env['PLATFORM'] + '|' + env['CONFIGURATION'] + ')'
        return '%s: %s%s%s\n' % (name, src_str, sep, tgt_str)

    def runnable_status(self):
        """
        Compute the task signature to make sure the scanner was executed. Create the
        moc tasks by using :py:meth:`waflib.Tools.qt5.qxx.add_moc_tasks` (if necessary),
        then postpone the task execution (there is no need to recompute the task signature).
        """
        status = Task.Task.runnable_status(self)
        if not self.moc_done:
            # ask the task if it needs to rebuild.  This will include checking all dependencies (.moc files included)
            # that may have changed.  If the task doesn't need to rebuild, no point in scanning for all the individual
            # moc tasks that need to be added
            if status != Task.RUN_ME:
                return status

            self.add_moc_tasks()
            # At this point, the moc task should be done, recycle and try the status check again
            self.moc_done = 1
            return Task.ASK_LATER

        return status

    def create_moc_task(self, h_node, moc_filename):
        """
        If several libraries use the same classes, it is possible that moc will run several times (Issue 1318)
        It is not possible to change the file names, but we can assume that the moc transformation will be identical,
        and the moc tasks can be shared in a global cache.

        The defines passed to moc will then depend on task generator order. If this is not acceptable, then
        use the tool slow_qt5 instead (and enjoy the slow builds... :-( )
        """

        cache_key = '{}.{}'.format(h_node.abspath(),self.generator.target_uid)

        try:
            moc_cache = self.generator.bld.moc_cache
        except AttributeError:
            moc_cache = self.generator.bld.moc_cache = {}

        try:
            return moc_cache[cache_key]
        except KeyError:
            relpath_target = os.path.join(h_node.parent.relpath(), moc_filename)

            target_node = change_target_qt5_node(self.generator.bld,
                                                 self.generator.path,
                                                 self.generator.name,
                                                 relpath_target,
                                                 self.generator.target_uid)

            tsk = moc_cache[cache_key] = Task.classes['moc'](env=self.env, generator=self.generator)
            tsk.set_inputs(h_node)
            tsk.set_outputs(target_node)

            self.dep_moc_files[target_node] = False

            if self.generator:
                self.generator.tasks.append(tsk)

            # direct injection in the build phase (safe because called from the main thread)
            gen = self.generator.bld.producer
            gen.outstanding.insert(0, tsk)
            gen.total += 1

            return tsk

    def moc_h_ext(self):
        try:
            ext = Options.options.qt_header_ext.split()
        except AttributeError:
            pass
        if not ext:
            ext = MOC_H
        return ext

    def add_moc_tasks(self):

        node = self.inputs[0]
        src_scan = node.read()

        # Determine if this is an uber file to see if we need to go one level deeper
        if src_scan.startswith(UBER_HEADER_COMMENT):
            # This is an uber file, handle uber files differently
            self.add_moc_task_uber(node,src_scan)
        else:
            # Process the source file (for mocs)
            self.add_moc_tasks_for_node(node,src_scan)

        del src_scan #free up the text as soon as possible

    def scan_node_contents_for_moc_tasks(self,node_contents):

        base_node = self.generator.path
        include_moc_node_rel_paths = INCLUDE_MOC_RE.findall(node_contents)
        moctasks = []

        for include_moc_node_rel_path in include_moc_node_rel_paths:

            base_name = os.path.splitext(include_moc_node_rel_path)[0]

            # We are only allowing to include mocing header files that are relative to the project folder
            header_node = None
            for moc_ext in self.moc_h_ext():
                # use search_node(), it will not create a node if the node is not found, and won't create bogus nodes while searching
                header_node = base_node.search_node('{}{}'.format(base_name, moc_ext))
                if header_node:
                    break
            if not header_node:
                raise Errors.WafError('No source found for {} which is a moc file.  Is the file included in .waf_files?'.format(base_name))

            moc_filename = '{}.moc'.format(os.path.splitext(header_node.name)[0])

            # create the moc task
            task = self.create_moc_task(header_node, moc_filename)
            moctasks.append(task)

        return moctasks

    def add_moc_task_uber(self, node, node_contents):
        '''
        Handle uber files by grepping for all the includes of source files and performing the moc scanning there
        '''
        moctasks = []
        include_source_rel_paths = INCLUDE_SRC_RE.findall(node_contents)
        for include_source_rel_path, include_source_extension in include_source_rel_paths:
            source_node = node.parent.find_node(include_source_rel_path)

            if source_node is None:
                source_node = self.generator.path.find_node(include_source_rel_path)

            if source_node is not None:
                source_node_contents = source_node.read()
                moctasks += self.scan_node_contents_for_moc_tasks(source_node_contents)
                del source_node_contents #free up the text as soon as possible


        # simple scheduler dependency: run the moc task before others
        self.run_after.update(set(moctasks))


    def add_moc_tasks_for_node(self, node, node_contents):
        '''
        Create the moc tasks greping the source file for all the #includes
        '''

        moctasks = self.scan_node_contents_for_moc_tasks(node_contents)

        # simple scheduler dependency: run the moc task before others
        self.run_after.update(set(moctasks))

class trans_update(Task.Task):
    """Update a .ts files from a list of C++ files"""
    run_str = '${QT_LUPDATE} ${SRC} -ts ${TGT}'
    color   = 'BLUE'
Task.update_outputs(trans_update)

class XMLHandler(ContentHandler):
    """
    Parser for *.qrc* files
    """
    def __init__(self):
        self.buf = []
        self.files = []
    def startElement(self, name, attrs):
        if name == 'file':
            self.buf = []
    def endElement(self, name):
        if name == 'file':
            self.files.append(str(''.join(self.buf)))
    def characters(self, cars):
        self.buf.append(cars)

@extension(*EXT_RCC)
def create_rcc_task(self, node):
    "Create rcc and cxx tasks for *.qrc* files"

    # Do not create tasks for project_generation builds
    if self.env['PLATFORM'] == 'project_generator':
        return None
    # Do not perform any task if QMAKE is not installed
    if not self.env.QMAKE:
        return None

    # For QRC Processing, we cannot make the generated rcc file from the qrc source as a separate compile unit
    # when creating static libs.  It appears that MSVC will optimize the required static methods required to
    # initialize the resources for the static lib.  In order to work around this, the generated file from the
    # qrc will need to be created as a header and included into a cpp that is consumed by the app/shared library
    # that is linking against it
    is_static_lib = 'stlib' == getattr(self,'_type','')

    if not getattr(self, 'rcc_tasks', False):
        self.rcc_tasks = []

    if is_static_lib:

        rcc_filename = 'rcc_%s.h' % os.path.splitext(node.name)[0]
        relpath_target = os.path.join(node.parent.relpath(), rcc_filename)

        rcnode = change_target_qt5_node(self.bld,
                                        self.path,
                                        self.name,
                                        relpath_target,
                                        self.target_uid)

        qrc_task = self.create_task('rcc', node, rcnode)
        self.rcc_tasks.append(qrc_task)
        return qrc_task
    else:

        rcc_filename = '%s_rc.cpp' % os.path.splitext(node.name)[0]
        relpath_target = os.path.join(node.parent.relpath(), rcc_filename)

        rcnode = change_target_qt5_node(self.bld,
                                        self.path,
                                        self.name,
                                        relpath_target,
                                        self.target_uid)

        qrc_task = self.create_task('rcc', node, rcnode)
        self.rcc_tasks.append(qrc_task)
        cpptask = self.create_task('cxx', rcnode, rcnode.change_ext('.o'))
        cpptask.dep_nodes.append(node)
        cpptask.set_run_after(qrc_task)
        try:
            self.compiled_tasks.append(cpptask)
        except AttributeError:
            self.compiled_tasks = [cpptask]
        return cpptask

@feature('qt5')
@after_method('process_source')
def add_rcc_dependencies(self):
    # are there rcc tasks?
    if not getattr(self, 'rcc_tasks', False):
        return

    rcc_tasks = set(self.rcc_tasks)
    for task in self.tasks:
        if any(isinstance(task, Task.classes[c]) for c in ['qxx', 'cxx', 'c']):
            task.run_after |= rcc_tasks


@feature('qt5')
@after_method('apply_link')
def create_automoc_task(self):
    if hasattr(self, 'header_files') and len(self.header_files) > 0:
        header_nodes = self.to_nodes(self.header_files)

        task = self.create_task('automoc', header_nodes)

        # this may mutate the link task, must run the link task after this task
        self.link_task.set_run_after(task)

@extension(*EXT_UI)
def create_uic_task(self, node):
    "hook for uic tasks"

    # Do not create tasks for project_generation builds
    if self.env['PLATFORM'] == 'project_generator':
        return None
    if not self.env.QMAKE:
        return None

    if not getattr(self, 'uic_tasks', False):
        self.uic_tasks = []

    uictask = self.create_task('ui5', node)

    ui_filename = self.env['ui_PATTERN'] % node.name[:-3]
    relpath_target = os.path.join(node.parent.relpath(), ui_filename)
    target_node = change_target_qt5_node(self.bld,
                                         self.path,
                                         self.name,
                                         relpath_target,
                                         self.target_uid)
    uictask.outputs = [target_node]
    self.uic_tasks.append(uictask)

@feature('qt5')
@after_method('process_source')
def add_uic_dependencies(self):
    # are there uic tasks?
    if not getattr(self, 'uic_tasks', False):
        return

    uic_tasks = set(self.uic_tasks)
    for task in self.tasks:
        if task.__class__.__name__ in ['qxx', 'cxx', 'c']:
            task.run_after |= uic_tasks

@extension('.ts')
def add_lang(self, node):
    """add all the .ts file into self.lang"""
    self.lang = self.to_list(getattr(self, 'lang', [])) + [node]

@feature('qt5')
@before_method('apply_incpaths')
def apply_qt5_includes(self):

    # Make sure the QT is enabled, otherwise whatever module is using this feature will fail
    if not self.env.QMAKE:
        return

    base_moc_node = get_target_qt5_root(self.bld,
                                        self.name,
                                        self.target_uid)
    if not hasattr(self, 'includes'):
        self.includes = []
    self.includes.append(base_moc_node)

    if self.env.PLATFORM == 'win_x64_clang':
        self.env.append_unique('CXXFLAGS', '-Wno-ignored-pragmas')


@feature('qt5')
@after_method('set_link_outputs')
def apply_qt5(self):
    """
    Add MOC_FLAGS which may be necessary for moc::

        def build(bld):
            bld.program(features='qt5', source='main.cpp', target='app', use='QTCORE')

    The additional parameters are:

    :param lang: list of translation files (\*.ts) to process
    :type lang: list of :py:class:`waflib.Node.Node` or string without the .ts extension
    :param update: whether to process the C++ files to update the \*.ts files (use **waf --translate**)
    :type update: bool
    :param langname: if given, transform the \*.ts files into a .qrc files to include in the binary file
    :type langname: :py:class:`waflib.Node.Node` or string without the .qrc extension
    """
    # Make sure the QT is enabled, otherwise whatever module is using this feature will fail
    if not self.env.QMAKE:
        return

    # If no type is defined, this is just a stub task that shouldn't handle any additional build/link tasks
    if not hasattr(self,'_type'):
        return

    if getattr(self, 'lang', None):
        qmtasks = []
        for x in self.to_list(self.lang):
            if isinstance(x, str):
                x = self.path.find_resource(x + '.ts')
            qm_filename = '%s.qm' % os.path.splitext(x.name)[0]
            relpath_target = os.path.join(x.parent.relpath(), qm_filename)
            new_qm_node = change_target_qt5_node(self.bld,
                                                 self.path,
                                                 self.name,
                                                 relpath_target,
                                                 self.target_uid)
            qmtask = self.create_task('ts2qm', x, new_qm_node)
            qmtasks.append(qmtask)

        if getattr(self, 'update', None) and Options.options.trans_qt5:
            cxxnodes = [a.inputs[0] for a in self.compiled_tasks] + [
                a.inputs[0] for a in self.tasks if getattr(a, 'inputs', None) and a.inputs[0].name.endswith('.ui')]
            for x in qmtasks:
                self.create_task('trans_update', cxxnodes, x.inputs)

        if getattr(self, 'langname', None):
            qmnodes = [x.outputs[0] for x in qmtasks]
            assert(isinstance(self.langname, str))
            qrc_filename = '%s.qrc' % self.langname
            relpath_target = os.path.join(self.path.relpath(), qrc_filename)
            new_rc_node = change_target_qt5_node(self.bld,
                                                 self.path,
                                                 self.name,
                                                 relpath_target,
                                                 self.target_uid)
            t = self.create_task('qm2rcc', qmnodes, new_rc_node)

            for x in qmtasks:
                t.set_run_after(x)

            k = create_rcc_task(self, t.outputs[0])

            if k:
                self.link_task.inputs.append(k.outputs[0])
                k.set_run_after(t)

    lst = []
    for flag in self.to_list(self.env['CXXFLAGS']):
        if len(flag) < 2: continue
        f = flag[0:2]
        if f in ('-D', '-I', '/D', '/I'):
            if f[0] == '/':
                lst.append('-' + flag[1:])
            else:
                lst.append(flag)

    if len(self.env['DEFINES']) > 0:
        for defined_value in self.env['DEFINES']:
            lst.append( '-D'+defined_value )

    # Apply additional QT defines for all MOCing
    additional_flags = ['-DQT_LARGEFILE_SUPPORT',
                        '-DQT_DLL',
                        '-DQT_CORE_LIB',
                        '-DQT_GUI_LIB']

    for additional_flag in  additional_flags:
        if not lst.__contains__(additional_flag):
            lst.append(additional_flag)


    self.env.append_value('MOC_FLAGS', lst)


@extension(*EXT_QT5)
def cxx_hook(self, node):
    """
    Re-map C++ file extensions to the :py:class:`waflib.Tools.qt5.qxx` task.
    """
    if 'qt5' in self.features:
        return self.create_compiled_task('qxx', node)
    else:
        return self.create_compiled_task('cxx', node)


# QT tasks involve code generation, so we need to also check if the generated code is still there
class QtTask(Task.Task):
    def runnable_status(self):

        missing_output = False
        for output in self.outputs:
            if not os.path.exists(output.abspath()):
                missing_output = True
                break

        if missing_output:
            for t in self.run_after:
                if not t.hasrun:
                    return Task.ASK_LATER
            return Task.RUN_ME

        status = Task.Task.runnable_status(self)
        return status


class automoc(Task.Task):
    def create_moc_tasks(self, moc_headers):
        moc_names = set()
        for moc_header in moc_headers:
            moc_node_name = os.path.splitext(moc_header.name)[0]

            # Make sure we don't have two moc files with the same name
            suffix = None
            while (moc_node_name + ("_%i" % suffix if suffix else "")) in moc_names:
                suffix = suffix + 1 if suffix else 2
            if suffix:
                moc_node_name += "_%i" % suffix
            moc_names.add(moc_node_name)

            cpp_filename = '%s_moc.cpp' % moc_node_name
            relpath_target = os.path.join(moc_header.parent.relpath(), cpp_filename)

            moc_node = change_target_qt5_node(self.generator.bld,
                                              self.generator.path,
                                              self.generator.name,
                                              relpath_target,
                                              self.generator.target_uid)

            moc_task = self.generator.create_task('moc', moc_header, moc_node)
            # Include the precompiled header, if applicable
            if getattr(self.generator, 'pch_header', None) is not None:
                moc_task.env['MOC_FLAGS'] = moc_task.env['MOC_FLAGS'] + ['-b', self.generator.pch_header]

            cpp_task = self.generator.create_compiled_task('cxx', moc_node)
            # Ignore warnings in generated code
            is_msvc = cpp_task.env['CXX_NAME'] == 'msvc'
            moc_cxx_flags = [flag for flag in cpp_task.env['CXXFLAGS'] if not flag.startswith('/W' if is_msvc else '-W')]

            if is_msvc and '/EHsc' not in moc_cxx_flags:
                moc_cxx_flags.append('/EHsc')
            elif not is_msvc and '-w' not in moc_cxx_flags:
                moc_cxx_flags.append('-w')

            cpp_task.env['CXXFLAGS'] = moc_cxx_flags

            # Define Q_MOC_BUILD for the (rare) case where a header might need to check to see if it's been included by
            # a _moc file.
            cpp_task.env.append_unique('DEFINES', 'Q_MOC_BUILD')
            cpp_task.set_run_after(moc_task)

            # add cpp output to link task.
            # Modifying the task should be ok because the link task is already registered as a run_after of
            # the automoc task (this task), and runnable_status in run on the main thread
            self.generator.link_task.inputs.append(cpp_task.outputs[0])
            self.generator.link_task.set_run_after(cpp_task)

            # direct injection in the build phase (safe because runnable_status is only called from the main thread)
            producer = self.generator.bld.producer
            producer.outstanding.insert(0, moc_task)  # insert the moc_task, its ready to run
            producer.outstanding.append(cpp_task)  # append the cpp_task, it must wait for the moc task completion anyways
            producer.total += 2

    def runnable_status(self):
        # check if any of the inputs have changed, or the input list has changed, or the dependencies have changed
        status = Task.Task.runnable_status(self)

        moc_headers = []
        if Task.RUN_ME == status:
            # run the automoc scan to generate the up-to-date contents
            for header_node in self.inputs:
                header_contents = header_node.read()
                # For now, only work on headers that opt in with an AUTOMOC comment
                if "AUTOMOC" not in header_contents:
                    continue
                header_contents = c_preproc.re_cpp.sub(c_preproc.repl, header_contents)
                if QOBJECT_RE.search(header_contents):
                    moc_headers.append(header_node)

            # store on task, will be added to the node_deps in post_run
            self.moc_headers = moc_headers
        else:
            # signatures didn't change, grab the saved nodes
            moc_headers = self.generator.bld.node_deps[self.uid()]

        # build the qt tasks, and add them to the link task
        self.create_moc_tasks(moc_headers)

        return status

    def scan(self):
        moc_headers = self.generator.bld.node_deps.get(self.uid(), [])
        return (moc_headers, [])

    def post_run(self):
        self.generator.bld.node_deps[self.uid()] = getattr(self, 'moc_headers', [])
        Task.Task.post_run(self)


class rcc(QtTask):
    """
    Process *.qrc* files
    """
    color   = 'BLUE'
    run_str = '${QT_RCC} -name ${tsk.rcname()} ${SRC[0].abspath()} ${RCC_ST} -o ${TGT}'
    ext_in = ['.qrc']

    def __init__(self, *k, **kw):
        QtTask.__init__(self, *k, **kw)

    def rcname(self):
        return os.path.splitext(self.inputs[0].name)[0]

    def parse_deps(self):
        """Parse the *.qrc* files"""
        if not has_xml:
            Logs.error('no xml support was found, the rcc dependencies will be incomplete!')
            return

        parser = make_parser()
        curHandler = XMLHandler()
        parser.setContentHandler(curHandler)
        fi = open(self.inputs[0].abspath(), 'r')
        try:
            parser.parse(fi)
        finally:
            fi.close()

        self.rcc_deps_paths = curHandler.files

    def lookup_deps(self, root, deps_paths):
        nodes = []
        names = []

        for x in deps_paths:
            nd = root.find_resource(x)
            if nd:
                nodes.append(nd)
            else:
                names.append(x)
        return (nodes, names)

    def scan(self):
        resolved_nodes = self.generator.bld.node_deps.get(self.uid(), [])
        unresolved_names = self.generator.bld.raw_deps.get(self.uid(), [])
        return (resolved_nodes, unresolved_names)

    def post_run(self):
        self.parse_deps()

        # convert input dependency files to nodes.  Care must be taken in this block wrt thread safety because it creates nodes
        if 'msvcdeps' in sys.modules:
            # msvcdeps is run on the worker threads, it may conflict with generate_deps, which is also creating node at
            # compile time.  Defer to msvcdeps module to handle thread locking
            (nodes, names) = sys.modules['msvcdeps'].sync_lookup_deps(self.inputs[0].parent, self.rcc_deps_paths)
        else:
            (nodes, names) = self.lookup_deps(self.inputs[0].parent, self.rcc_deps_paths)

        del self.rcc_deps_paths

        # store dependencies in build
        self.generator.bld.node_deps[self.uid()] = nodes
        self.generator.bld.raw_deps[self.uid()] = names

        # delete signature to force a rebuild of signature.  Scan() will be called to store the deps
        try:
            del self.cache_sig
        except:
            pass

        # call base class to regenerate signature
        super(rcc, self).post_run()


class moc(QtTask):
    """
    Create *.moc* files
    """
    color   = 'BLUE'

    run_str = '${QT_MOC} ${MOC_FLAGS} ${SRC} -o ${TGT}'


class fake_moc(QtTask):
    """
    Create dummy *.moc files - this is a temporary workaround while we migrate to autmoc
    """
    color = 'BLUE'

    def post_run(self):
        self.outputs[0].write("/* Dummy moc file, this will eventually be removed */\n")
        super(fake_moc, self).post_run(self)


class ui5(QtTask):
    """
    Process *.ui* files
    """
    color   = 'BLUE'
    run_str = '${QT_UIC} ${SRC} -o ${TGT}'
    ext_in = ['.ui']

class ts2qm(QtTask):
    """
    Create *.qm* files from *.ts* files
    """
    color   = 'BLUE'
    run_str = '${QT_LRELEASE} ${QT_LRELEASE_FLAGS} ${SRC} -qm ${TGT}'

class qm2rcc(QtTask):
    """
    Transform *.qm* files into *.rc* files
    """
    color = 'BLUE'
    after = 'ts2qm'

    def run(self):
        """Create a qrc file including the inputs"""
        txt = '\n'.join(['<file>%s</file>' % k.path_from(self.outputs[0].parent) for k in self.inputs])
        code = '<!DOCTYPE RCC><RCC version="1.0">\n<qresource>\n%s\n</qresource>\n</RCC>' % txt
        self.outputs[0].write(code)


bin_cache = {}

# maintain a cache set of platforms that don't have Qt
# so that we don't needlessly search multiple times, and
# so that the user doesn't get numerous warnings of the same thing
QT_SDK_MISSING = Set()


@conf
def get_qt_version(self):
    # at the end, try to find qmake in the paths given
    # keep the one with the highest version
    version = None
    paths = []
    prev_ver = ['5', '0', '0']
    for qmk in ('qmake-qt5', 'qmake5', 'qmake'):
        try:
            qmake = self.find_program(qmk, path_list=paths, silent_output=True)
        except self.errors.ConfigurationError:
            pass
        else:
            try:
                version = self.cmd_and_log([qmake] + ['-query', 'QT_VERSION'], quiet=Context.BOTH).strip()
            except self.errors.WafError:
                version = None
                pass

    # qmake could not be found easily, rely on qtchooser
    if version is None:
        try:
            self.find_program('qtchooser', silent_output=True)
        except self.errors.ConfigurationError:
            pass
        else:
            cmd = [self.env.QTCHOOSER] + ['-qt=5', '-run-tool=qmake']
            try:
                version = self.cmd_and_log(cmd + ['-query', 'QT_VERSION'], quiet=Context.BOTH).strip()
            except self.errors.WafError:
                pass

    return version

def _prepare_lib_folder_for_linux(qt_lib_path):
    # this functions sets up the qt linux shared library, for example
    # libQt5Xml.so -> libQt5Xml.so.5.6.2
    # libQt5Xml.so.5 -> libQt5Xml.so.5.6.2
    # libQt5Xml.so.5.6 -> libQt5Xml.so.5.6.2
    import glob
    library_files = glob.glob(os.path.join(qt_lib_path, 'lib*.so*'))
    for lib_path in library_files:

        if os.path.islink(lib_path):
            continue

        lib_path_basename = os.path.basename(lib_path)

        new_lib_path, ext = os.path.splitext(lib_path)
        while ext != '.so':
            if os.path.lexists(new_lib_path) is False:
                os.symlink(lib_path_basename, new_lib_path)
                Logs.debug('Made link: {} -> {}'.format(lib_path, new_lib_path))
            new_lib_path, ext = os.path.splitext(new_lib_path)

@conf
def find_qt5_binaries(self, platform):

    # platform has to be passed in, as it hasn't been set in the env
    # when this function is called

    global QT_SDK_MISSING
    if platform in QT_SDK_MISSING:
        return False

    env = self.env

    opt = Options.options
    qtbin = getattr(opt, 'qtbin', '')

    platformDirectoryMapping = {
        'win_x64_vs2013': 'msvc2013_64',
        'win_x64_vs2015': 'msvc2015_64',
        'win_x64_vs2017': 'msvc2015_64',  # Not an error, VS2017 links with VS2015 binaries
        'win_x64_clang': 'msvc2015_64',
        'darwin_x64': 'clang_64',
        'linux_x64': 'gcc_64'
    }

    if not platformDirectoryMapping.has_key(platform):
        self.fatal('Platform %s is not supported by our Qt waf scripts!' % platform)

    # Get the QT dir from the third party settings
    qtdir, enabled, roles, _ = self.tp.get_third_party_path(platform, 'qt')

    # If the path was not resolved, it could be an invalid alias (missing from the SetupAssistantConfig.json)
    if not qtdir:
        raise Errors.WafError("Invalid required QT alias for platform {}".format(platform))

    # If the path was resolved, we still need to make sure the 3rd party is enabled based on the roles
    if not enabled:
        error_message = "Unable to resolve Qt because it is not enabled in Setup Assistant.  \nMake sure that at least " \
                        "one of the following roles is enabled: [{}]".format(', '.join(roles))
        raise Errors.WafError(error_message)

    qtdir = os.path.join(qtdir, platformDirectoryMapping[platform])

    paths = []

    if qtdir:
        qtbin = os.path.join(qtdir, 'bin')

    # the qt directory has been given from QT5_ROOT - deduce the qt binary path
    if not qtdir:
        qtdir = os.environ.get('QT5_ROOT', '')
        qtbin = os.environ.get('QT5_BIN', None) or os.path.join(qtdir, 'bin')

    if qtbin:
        paths = [qtbin]

    qmake_cache_key = qtdir + '_QMAKE'
    if qmake_cache_key in bin_cache:
        self.env.QMAKE = bin_cache[qmake_cache_key]
    else:

        # at the end, try to find qmake in the paths given
        # keep the one with the highest version
        cand = None
        prev_ver = ['5', '0', '0']
        for qmk in ('qmake-qt5', 'qmake5', 'qmake'):
            try:
                qmake = self.find_program(qmk, path_list=paths, silent_output=True)
            except self.errors.ConfigurationError:
                pass
            else:
                try:
                    version = self.cmd_and_log([qmake] + ['-query', 'QT_VERSION']).strip()
                except self.errors.WafError:
                    pass
                else:
                    if version:
                        new_ver = version.split('.')
                        if new_ver > prev_ver:
                            cand = qmake
                            prev_ver = new_ver

        # qmake could not be found easily, rely on qtchooser
        if not cand:
            try:
                self.find_program('qtchooser')
            except self.errors.ConfigurationError:
                pass
            else:
                cmd = [self.env.QTCHOOSER] + ['-qt=5', '-run-tool=qmake']
                try:
                    version = self.cmd_and_log(cmd + ['-query', 'QT_VERSION'])
                except self.errors.WafError:
                    pass
                else:
                    cand = os.path.normpath(cmd)

        if cand:
            self.env.QMAKE = cand
            bin_cache[qmake_cache_key] = cand
        else:
            # If we cannot find qmake, we will assume that QT is not available or a selected option
            # Therefore, we cannot build the lumberyard editor and tools
            Logs.warn('[WARN] Unable to find the appropriate QT library.  Make sure you have QT installed if you wish to compile the Lumberyard Editor and tools.')
            QT_SDK_MISSING.add(platform)

            return False

    qmake_cache_key = qtdir + '_QT_INSTALL_BINS'
    if qmake_cache_key in bin_cache:
        self.env.QT_INSTALL_BINS = qtbin = bin_cache[qmake_cache_key]
    else:
        query_qt_bin_result = self.cmd_and_log([self.env.QMAKE] + ['-query', 'QT_INSTALL_BINS']).strip() + os.sep
        self.env.QT_INSTALL_BINS = qtbin = os.path.normpath(query_qt_bin_result) + os.sep
        bin_cache[qmake_cache_key] = qtbin

    paths.insert(0, qtbin)

    def _get_qtlib_subfolder(name):
        qt_subdir = os.path.join(qtdir, name)
        if not os.path.exists(qt_subdir):
            self.fatal('Unable to find QT lib folder {}'.format(name))
        return qt_subdir

    # generate symlinks for the library files within the lib folder
    if platform == "linux_x64":
        _prepare_lib_folder_for_linux(_get_qtlib_subfolder("lib"))

    def find_bin(lst, var):

        if var in env:
            return

        cache_key = qtdir + '_' + var
        if cache_key in bin_cache:
            env[var] = bin_cache[cache_key]
            return

        for f in lst:
            try:
                ret = self.find_program(f, path_list=paths, silent_output=True)
            except self.errors.ConfigurationError:
                pass
            else:
                env[var] = os.path.normpath(ret)
                bin_cache[cache_key] = os.path.normpath(ret)
                break

    find_bin(['uic-qt5', 'uic'], 'QT_UIC')
    if not env.QT_UIC:
        # If we find qmake but not the uic compiler, then the QT installation is corrupt/invalid
        self.fatal('Detected an invalid/corrupt version of QT, please check your installation')
    uic_version_cache_key = qtdir + '_UICVERSION'
    if uic_version_cache_key not in bin_cache:
        uicver = self.cmd_and_log([env.QT_UIC] + ['-version'], output=Context.BOTH, quiet=True)
        uicver = ''.join(uicver).strip()
        uicver = uicver.replace('Qt User Interface Compiler ','').replace('User Interface Compiler for Qt', '')
        if uicver.find(' 3.') != -1 or uicver.find(' 4.') != -1:
            self.fatal('this uic compiler is for qt3 or qt5, add uic for qt5 to your path')
        bin_cache[uic_version_cache_key] = uicver

    find_bin(['moc-qt5', 'moc'], 'QT_MOC')
    find_bin(['rcc-qt5', 'rcc'], 'QT_RCC')
    find_bin(['lrelease-qt5', 'lrelease'], 'QT_LRELEASE')
    find_bin(['lupdate-qt5', 'lupdate'], 'QT_LUPDATE')

    env['UIC_ST'] = '%s -o %s'
    env['MOC_ST'] = '-o'
    env['ui_PATTERN'] = 'ui_%s.h'
    env['QT_LRELEASE_FLAGS'] = ['-silent']
    env.MOCCPPPATH_ST = '-I%s'
    env.MOCDEFINES_ST = '-D%s'

    env.QT_BIN_DIR = _get_qtlib_subfolder('bin')
    env.QT_LIB_DIR = _get_qtlib_subfolder('lib')
    env.QT_QML_DIR = _get_qtlib_subfolder('qml')
    env.QT_PLUGINS_DIR = _get_qtlib_subfolder('plugins')

    return True

@conf
def find_qt5_libraries(self):
    qtlibs = getattr(Options.options, 'qtlibs', None) or os.environ.get("QT5_LIBDIR", None)
    if not qtlibs:
        try:
            qtlibs = self.cmd_and_log([self.env.QMAKE] + ['-query', 'QT_INSTALL_LIBS']).strip()
        except Errors.WafError:
            qtdir = self.cmd_and_log([self.env.QMAKE] + ['-query', 'QT_INSTALL_PREFIX']).strip() + os.sep
            qtlibs = os.path.join(qtdir, 'lib')
    self.msg('Found the Qt5 libraries in', qtlibs)

    qtincludes =  os.environ.get("QT5_INCLUDES", None) or self.cmd_and_log([self.env.QMAKE] + ['-query', 'QT_INSTALL_HEADERS']).strip()
    env = self.env
    if not 'PKG_CONFIG_PATH' in os.environ:
        os.environ['PKG_CONFIG_PATH'] = '%s:%s/pkgconfig:/usr/lib/qt5/lib/pkgconfig:/opt/qt5/lib/pkgconfig:/usr/lib/qt5/lib:/opt/qt5/lib' % (qtlibs, qtlibs)

    if Utils.unversioned_sys_platform() == "darwin":
        if qtlibs:
            env.append_unique('FRAMEWORKPATH',qtlibs)

    # Keep track of platforms that were checked (there is no need to do a multiple report)
    checked_darwin = False
    checked_linux = False
    checked_win_x64 = False

    validated_platforms = self.get_available_platforms()
    for validated_platform in validated_platforms:

        is_platform_darwin = validated_platform in (['darwin_x64', 'ios', 'appletv'])
        is_platform_linux = validated_platform in (['linux_x64_gcc'])
        is_platform_win_x64 = validated_platform.startswith('win_x64')

        for i in self.qt5_vars:
            uselib = i.upper()

            # Platform is darwin_x64 / mac
            if is_platform_darwin:

                # QT for darwin does not have '5' in the name, so we need to remove it
                darwin_adjusted_name = i.replace('Qt5','Qt')

                # Since at least qt 4.7.3 each library locates in separate directory
                frameworkName = darwin_adjusted_name + ".framework"
                qtDynamicLib = os.path.join(qtlibs, frameworkName, darwin_adjusted_name)
                if os.path.exists(qtDynamicLib):
                    env.append_unique('FRAMEWORK_{}_{}'.format(validated_platform,uselib), darwin_adjusted_name)
                    if not checked_darwin:
                        self.msg('Checking for %s' % i, qtDynamicLib, 'GREEN')
                else:
                    if not checked_darwin:
                        self.msg('Checking for %s' % i, False, 'YELLOW')
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib), os.path.join(qtlibs, frameworkName, 'Headers'))

                # Detect the debug versions of the library
                uselib_debug = i.upper() + "D"
                darwin_adjusted_name_debug = '{}_debug'.format(darwin_adjusted_name)
                qtDynamicLib_debug = os.path.join(qtlibs, frameworkName, darwin_adjusted_name_debug)
                if os.path.exists(qtDynamicLib_debug):
                    env.append_unique('FRAMEWORK_{}_{}'.format(validated_platform, uselib_debug), darwin_adjusted_name)
                    if not checked_darwin:
                        self.msg('Checking for %s_debug' % i, qtDynamicLib_debug, 'GREEN')
                else:
                    if not checked_darwin:
                        self.msg('Checking for %s_debug' % i, False, 'YELLOW')
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib_debug), os.path.join(qtlibs, frameworkName, 'Headers'))

            # Platform is linux+gcc
            elif is_platform_linux:

                qtDynamicLib = os.path.join(qtlibs, "lib" + i + ".so")
                qtStaticLib = os.path.join(qtlibs, "lib" + i + ".a")
                if os.path.exists(qtDynamicLib):
                    env.append_unique('LIB_{}_{}'.format(validated_platform,uselib), i)
                    if not checked_linux:
                        self.msg('Checking for %s' % i, qtDynamicLib, 'GREEN')
                elif os.path.exists(qtStaticLib):
                    env.append_unique('LIB_{}_{}'.format(validated_platform,uselib), i)
                    if not checked_linux:
                        self.msg('Checking for %s' % i, qtStaticLib, 'GREEN')
                else:
                    if not checked_linux:
                        self.msg('Checking for %s' % i, False, 'YELLOW')

                env.append_unique('LIBPATH_{}_{}'.format(validated_platform,uselib), qtlibs)
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib), qtincludes)
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib), os.path.join(qtincludes, i))

            # Platform is win_x64
            elif is_platform_win_x64:
                # Release library names are like QtCore5
                for k in ("lib%s.a", "lib%s5.a", "%s.lib", "%s5.lib"):
                    lib = os.path.join(qtlibs, k % i)
                    if os.path.exists(lib):
                        env.append_unique('LIB_{}_{}'.format(validated_platform,uselib), i + k[k.find("%s") + 2 : k.find('.')])
                        if not checked_win_x64:
                            self.msg('Checking for %s' % i, lib, 'GREEN')
                        break
                else:
                    if not checked_win_x64:
                        self.msg('Checking for %s' % i, False, 'YELLOW')

                env.append_unique('LIBPATH_{}_{}'.format(validated_platform,uselib), qtlibs)
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib), qtincludes)
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib), os.path.join(qtincludes, i.replace('Qt5', 'Qt')))

                # Debug library names are like QtCore5d
                uselib = i.upper() + "D"
                for k in ("lib%sd.a", "lib%sd5.a", "%sd.lib", "%sd5.lib"):
                    lib = os.path.join(qtlibs, k % i)
                    if os.path.exists(lib):
                        env.append_unique('LIB_{}_{}'.format(validated_platform,uselib), i + k[k.find("%s") + 2 : k.find('.')])
                        if not checked_win_x64:
                            self.msg('Checking for %s' % i, lib, 'GREEN')
                        break
                else:
                    if not checked_win_x64:
                        self.msg('Checking for %s' % i, False, 'YELLOW')

                env.append_unique('LIBPATH_{}_{}'.format(validated_platform,uselib), qtlibs)
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib), qtincludes)
                env.append_unique('INCLUDES_{}_{}'.format(validated_platform,uselib), os.path.join(qtincludes, i.replace('Qt5', 'Qt')))
            else:
                # The current target platform is not supported for QT5
                Logs.debug('lumberyard: QT5 detection not supported for platform {}'.format(validated_platform))
                pass

        if is_platform_darwin:
            checked_darwin = True
        elif is_platform_linux:
            checked_linux = True
        elif is_platform_win_x64:
            checked_win_x64 = True

@conf
def simplify_qt5_libs(self):
    # the libpaths make really long command-lines
    # remove the qtcore ones from qtgui, etc
    env = self.env
    def process_lib(vars_, coreval):
        validated_platforms = self.get_available_platforms()
        for validated_platform in validated_platforms:
            for d in vars_:
                var = d.upper()
                if var == 'QTCORE':
                    continue

                value = env['LIBPATH_{}_{}'.format(validated_platform, var)]
                if value:
                    core = env[coreval]
                    accu = []
                    for lib in value:
                        if lib in core:
                            continue
                        accu.append(lib)
                    env['LIBPATH_{}_{}'.format(validated_platform, var)] = accu

    process_lib(self.qt5_vars,       'LIBPATH_QTCORE')
    process_lib(self.qt5_vars_debug, 'LIBPATH_QTCORE_DEBUG')

@conf
def add_qt5_rpath(self):
    # rpath if wanted
    env = self.env
    if getattr(Options.options, 'want_rpath', False):
        def process_rpath(vars_, coreval):

            validated_platforms = self.get_available_platforms()
            for validated_platform in validated_platforms:
                for d in vars_:
                    var = d.upper()
                    value = env['LIBPATH_{}_{}'.format(validated_platform, var)]
                    if value:
                        core = env[coreval]
                        accu = []
                        for lib in value:
                            if var != 'QTCORE':
                                if lib in core:
                                    continue
                            accu.append('-Wl,--rpath='+lib)
                        env['RPATH_{}_{}'.format(validated_platform, var)] = accu
        process_rpath(self.qt5_vars,       'LIBPATH_QTCORE')
        process_rpath(self.qt5_vars_debug, 'LIBPATH_QTCORE_DEBUG')

@conf
def set_qt5_libs_to_check(self):
    if not hasattr(self, 'qt5_vars'):
        self.qt5_vars = QT5_LIBS
    self.qt5_vars = Utils.to_list(self.qt5_vars)
    if not hasattr(self, 'qt5_vars_debug'):
        self.qt5_vars_debug = [a + '_debug' for a in self.qt5_vars]
    self.qt5_vars_debug = Utils.to_list(self.qt5_vars_debug)

@conf
def set_qt5_defines(self):
    if sys.platform != 'win32':
        return

    validated_platforms = self.get_available_platforms()
    for validated_platform in validated_platforms:
        for x in self.qt5_vars:
            y=x.replace('Qt5', 'Qt')[2:].upper()
            self.env.append_unique('DEFINES_{}_{}'.format(validated_platform,x.upper()), 'QT_%s_LIB' % y)
            self.env.append_unique('DEFINES_{}_{}_DEBUG'.format(validated_platform,x.upper()), 'QT_%s_LIB' % y)

def options(opt):
    """
    Command-line options
    """
    opt.add_option('--want-rpath', action='store_true', default=False, dest='want_rpath', help='enable the rpath for qt libraries')

    opt.add_option('--header-ext',
        type='string',
        default='',
        help='header extension for moc files',
        dest='qt_header_ext')

    for i in 'qtdir qtbin qtlibs'.split():
        opt.add_option('--'+i, type='string', default='', dest=i)

    opt.add_option('--translate', action="store_true", help="collect translation strings", dest="trans_qt5", default=False)

SUPPORTED_QTLIB_PLATFORMS = ['win_x64_vs2013', 'win_x64_vs2015', 'win_x64_vs2017', 'win_x64_clang', 'darwin_x64', 'linux_x64']

PLATFORM_TO_QTGA_SUBFOLDER = {
    "win_x64_vs2013": ["win32/vc120/qtga.dll", "win32/vc120/qtgad.dll", "win32/vc120/qtgad.pdb"],
    "win_x64_vs2015": ["win32/vc140/qtga.dll", "win32/vc140/qtgad.dll", "win32/vc140/qtgad.pdb"],
    "win_x64_vs2017": ["win32/vc140/qtga.dll", "win32/vc140/qtgad.dll", "win32/vc140/qtgad.pdb"],  # Not an error, VS2017 links with VS2015 binaries
    "win_x64_clang": ["win32/vc140/qtga.dll", "win32/vc140/qtgad.dll", "win32/vc140/qtgad.pdb"],
    "darwin_x64": ["macx/libqtga.dylib", "macx/libqtga_debug.dylib"],
    "linux_x64": []
}

IGNORE_QTLIB_PATTERNS = [
    # cmake Not needed
    os.path.normcase('lib/cmake'),

    # Special LY built plugins that will be copied from a different source
    'qtga.dll',
    'qtga.pdb',
    'qtgad.dll',
    'qtgad.pdb',
    'libqttga.dylib',
    'libqttga_debug.dylib'
]

ICU_DLLS = [
    "icudt54",
    "icuin54",
    "icuuc54"
]

WINDOWS_RC_QT_DLLS = [
    "Qt5Core",
    "Qt5Gui",
    "Qt5Network",
    "Qt5Qml",
    "Qt5Quick",
    "Qt5Svg",
    "Qt5Widgets",
]

WINDOWS_LMBRSETUP_QT_DLLS = [
    "Qt5Core",
    "Qt5Gui",
    "Qt5Network",
    "Qt5Qml",
    "Qt5Quick",
    "Qt5Svg",
    "Qt5Widgets",
    "Qt5Concurrent",
    "Qt5WinExtras",
    "libEGL",
    "libGLESv2"
]

WINDOWS_MAIN_QT_DLLS = [
    "Qt5Core",
    "Qt5Gui",
    "Qt5Network",
    "Qt5Qml",
    "Qt5Quick",
    "Qt5Svg",
    "Qt5Widgets",
    "Qt5Bluetooth",
    "Qt5CLucene",
    "Qt5Concurrent",
    "Qt5DBus",
    "Qt5DesignerComponents",
    "Qt5Designer",
    "Qt5Help",
    "Qt5MultimediaQuick_p",
    "Qt5Multimedia",
    "Qt5MultimediaWidgets",
    "Qt5Nfc",
    "Qt5OpenGL",
    "Qt5Positioning",
    "Qt5PrintSupport",
    "Qt5QuickParticles",
    "Qt5QuickTest",
    "Qt5Script",
    "Qt5ScriptTools",
    "Qt5Sensors",
    "Qt5SerialPort",
    "Qt5Sql",
    "Qt5Test",
    "Qt5WebChannel",
    "Qt5WebKit",
    "Qt5WebKitWidgets",
    "Qt5WinExtras",
    "Qt5XmlPatterns",
    "Qt5Xml",
    "libEGL",
    "libGLESv2"
]

@conf
def qtlib_bootstrap(self, platform, configuration):

    global QT_SDK_MISSING
    if platform in QT_SDK_MISSING:
        return

    def _copy_folder(src, dst, qt_type, pattern, is_ignore):
        dst_type = os.path.normcase(os.path.join(dst, qt_type))
        return copy_tasks.copy_tree2(src, dst_type, False, pattern, is_ignore, False)

    def _copy_file(src_path, dest_path):
        src = os.path.normcase(src_path)
        dst = os.path.normcase(dest_path)
        copy_file = copy_tasks.should_overwrite_file(src, dst)
        if copy_file:

            try:
                # In case the file is readonly, we'll remove the existing file first
                if os.path.exists(dst):
                    os.chmod(dst, stat.S_IWRITE)
            except Exception as err:
                Logs.warn('[WARN] Unable to make target file {} writable {}'.format(dst, err.message))

            try:
                shutil.copy2(src, dst)
            except Exception as err:
                Logs.warn('[WARN] Unable to copy {} to {}: {}'.format(src, dst, err.message))

            return 1
        else:
            return 0

    def _copy_dlls(qt_dlls_source, target_folder):

        if not os.path.exists(target_folder):
            os.makedirs(target_folder)
        copied = 0
        for qtdll in qt_dlls_source:
            src_dll = os.path.join(self.env.QT_BIN_DIR, qtdll)
            dst_dll = os.path.join(target_folder, qtdll)
            copied += _copy_file(src_dll, dst_dll)
        return copied

    def _copy_qtlib_folder(ctx, dst, current_platform, patterns, is_required_pattern):

        # Used to track number of files copied by this function
        num_files_copied = 0

        # Create the qtlibs subfolder
        dst_qtlib = os.path.normcase(os.path.join(dst, 'qtlibs'))
        if not os.path.exists(dst_qtlib):
            os.makedirs(dst_qtlib)

        # If qt fails to configure, the folder copies below will give meaningless errors.
        # Test for this condition and error out here
        if not ctx.env.QT_LIB_DIR:
            Logs.warn('unable to find QT')
            return num_files_copied

        # Copy the libs for qtlibs
        lib_pattern = patterns
        if 'lib' in patterns:
            lib_pattern = patterns['lib']
        num_files_copied += _copy_folder(ctx.env.QT_LIB_DIR, dst_qtlib, 'lib', lib_pattern, is_required_pattern)

        # special setup for linux_x64 platform
        if platform == 'linux_x64':
            _prepare_lib_folder_for_linux(os.path.join(dst_qtlib, 'lib'))

        # Copy the qml for qtlibs
        qml_pattern = patterns
        if 'qml' in patterns:
            qml_pattern = patterns['qml']
        num_files_copied += _copy_folder(ctx.env.QT_QML_DIR, dst_qtlib, 'qml', qml_pattern, is_required_pattern)

        # Copy the plugins for qtlibs
        plugins_pattern = patterns
        if 'plugins' in patterns:
            plugins_pattern = patterns['plugins']
        num_files_copied += _copy_folder(ctx.env.QT_PLUGINS_DIR, dst_qtlib, 'plugins', plugins_pattern, is_required_pattern)

        # Copy the extra txt files
        qt_base = os.path.normpath(ctx.ThirdPartyPath('qt',''))
        num_files_copied += _copy_file(os.path.join(qt_base, 'QT-NOTICE.TXT'),
                                   os.path.join(dst_qtlib, 'QT-NOTICE.TXT'))
        num_files_copied += _copy_file(os.path.join(qt_base, 'ThirdPartySoftware_Listing.txt'),
                                   os.path.join(dst_qtlib, 'ThirdPartySoftware_Listing.txt'))

        qt_tga_files = PLATFORM_TO_QTGA_SUBFOLDER.get(current_platform, [])
        qt_tga_src_root = os.path.normcase(ctx.Path('Tools/Redistributables/QtTgaImageFormatPlugin'))
        for qt_tga_file in qt_tga_files:

            if not is_copy_pdbs and qt_tga_file.endswith('.pdb'):
                continue

            source_qt_tga = os.path.normcase(os.path.join(qt_tga_src_root, qt_tga_file))
            dest_qt_tga = os.path.normcase(
                os.path.join(dst_qtlib, 'plugins/imageformats', os.path.basename(qt_tga_file)))
            num_files_copied += _copy_file(source_qt_tga, dest_qt_tga)

        return num_files_copied

    def _copy_qt_dlls(ctx, dst, copy_dll_list):
        debug_dll_fn = lambda qt: qt + ('d.dll' if is_debug else '.dll')
        ext_dll_fn = lambda dll: dll + '.dll'
        ext_pdb_fn = lambda pdb: pdb + '.pdb'

        qt_main_dlls = [debug_dll_fn(qt) for qt in copy_dll_list]
        qt_main_dlls += [ext_dll_fn(icu) for icu in ICU_DLLS]

        if is_debug and is_copy_pdbs:
            qt_main_dlls += [ext_pdb_fn(qt) for qt in copy_dll_list]

        num_files_copied = 0
        try:
            if not os.path.exists(ctx.env.QT_BIN_DIR):
                Logs.debug('Unable to locate QT Bin folder: {}.'.format(ctx.env.QT_BIN_DIR))
                QT_SDK_MISSING.add(platform)
                return num_files_copied
        except TypeError:
            Logs.debug('Unable to locate QT Bin folder.')
            QT_SDK_MISSING.add(platform)
            return num_files_copied

        # Copy the QT.dlls to the main configuration output folder
        num_files_copied += _copy_dlls(qt_main_dlls, dst)

        return num_files_copied

    is_copy_pdbs = self.is_option_true('copy_3rd_party_pdbs')

    output_paths = self.get_output_folders(platform, configuration)
    if len(output_paths) != 1:
        self.fatal('Assertion error: Multiple output paths returned')
    output_path = output_paths[0].abspath()
    if not os.path.exists(output_path):
        os.makedirs(output_path)

    # Check if current configuration is a debug build
    is_debug = configuration.startswith('debug')

    # For windows, we will bootstrap copy the Qt Dlls to the main and rc subfolder
    # (for non-test and non-dedicated configurations)
    if platform in ['win_x64_clang', 'win_x64_vs2017', 'win_x64_vs2015', 'win_x64_vs2013'] and configuration in \
            ['debug', 'profile', 'debug_test', 'profile_test', 'debug_dedicated', 'profile_dedicated']:

        copy_timer = Utils.Timer()

        # Check if current configuration is a debug build
        is_debug = configuration.startswith('debug')

        # Copy all the dlls required by Qt
        # Copy to the current configuration's BinXXX folder
        files_copied = _copy_qt_dlls(self, output_path, WINDOWS_MAIN_QT_DLLS)
        # Copy to the current configuration's BinXXX/rc folder
        files_copied += _copy_qt_dlls(self, os.path.join(output_path, 'rc'), WINDOWS_RC_QT_DLLS)
        # Copy to the LmbrSetup folder
        files_copied += _copy_qt_dlls(self,
            self.Path(self.get_lmbr_setup_tools_output_folder()), lmbr_setup_tools.LMBR_SETUP_QT_FILTERS['win']['Modules'])

        # Report the sync job, but only report the number of files if any were actually copied
        if files_copied > 0:
            Logs.info('[INFO] Copied Qt DLLs to target folder: {} files copied. ({})'
                      .format(files_copied, str(copy_timer)))
        else:
            if Logs.verbose > 1:
                Logs.info('[INFO] Skipped qt dll copy to target folder. ({})'.format(str(copy_timer)))

    # Check if this is a platform that supports the qtlib folder synchronization
    if platform in SUPPORTED_QTLIB_PLATFORMS:

        copy_timer = Utils.Timer()

        # Used as a pattern-set to ignore certain qt library files
        ignore_lib_patterns = IGNORE_QTLIB_PATTERNS if is_copy_pdbs else IGNORE_QTLIB_PATTERNS + ['.pdb']

        # Copy the entire qtlib folder to current output path
        # Contains lib, plugins and qml folders, and license information
        files_copied = _copy_qtlib_folder(self, output_path, platform, ignore_lib_patterns, False)

        lmbr_configuration_key = 'debug' if is_debug else 'profile'
        lmbr_platform_key = ''
        for key in lmbr_setup_tools.LMBR_SETUP_QT_FILTERS:
            if platform.startswith(key):
                lmbr_platform_key = key
                break

        if not lmbr_platform_key:
            Logs.error('Cannot find the current configuration ({}) to setup LmbrSetup folder.'.format(platform))

        files_copied += _copy_qtlib_folder(self,
            self.Path(self.get_lmbr_setup_tools_output_folder()),
            platform, lmbr_setup_tools.LMBR_SETUP_QT_FILTERS[lmbr_platform_key]['qtlibs'][lmbr_configuration_key], True)

        # Report the sync job, but only report the number of files if any were actually copied
        if files_copied > 0:
            Logs.info('[INFO] Copied qtlibs folder to target folder: {} files copied. ({})'
                      .format(files_copied, str(copy_timer)))
        else:
            if Logs.verbose > 1:
                Logs.info('[INFO] Copied qtlibs folder to target folder: No files copied. ({})'
                          .format(str(copy_timer)))





