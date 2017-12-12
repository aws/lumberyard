#!/usr/bin/env python
# encoding: utf-8
# Matthias Jahn jahn dôt matthias ât freenet dôt de, 2007 (pmarat)

"""
Try to detect a C compiler from the list of supported compilers (gcc, msvc, etc)::

	def options(opt):
		opt.load('compiler_c')
	def configure(cnf):
		cnf.load('compiler_c')
	def build(bld):
		bld.program(source='main.c', target='app')

The compilers are associated to platforms in :py:attr:`waflib.Tools.compiler_c.c_compiler`. To register
a new C compiler named *cfoo* (assuming the tool ``waflib/extras/cfoo.py`` exists), use::

	def options(opt):
		opt.load('compiler_c')
	def configure(cnf):
		from waflib.Tools.compiler_c import c_compiler
		c_compiler['win32'] = ['cfoo', 'msvc', 'gcc']
		cnf.load('compiler_c')
	def build(bld):
		bld.program(source='main.c', target='app')

Not all compilers need to have a specific tool. For example, the clang compilers can be detected by the gcc tools when using::

	$ CC=clang waf configure
"""

import os, sys, imp, types
from waflib.Tools import ccroot
from waflib import Utils, Configure
from waflib.Logs import debug

c_compiler = {
'win32':  ['msvc', 'gcc'],
'cygwin': ['gcc'],
'darwin': ['gcc'],
'aix':    ['xlc', 'gcc'],
'linux':  ['gcc', 'icc'],
'sunos':  ['suncc', 'gcc'],
'irix':   ['gcc', 'irixcc'],
'hpux':   ['gcc'],
'gnu':    ['gcc'],
'java':   ['gcc', 'msvc', 'icc'],
'default':['gcc'],
}
"""
Dict mapping the platform names to waf tools finding specific compilers::

	from waflib.Tools.compiler_c import c_compiler
	c_compiler['linux'] = ['gcc', 'icc', 'suncc']
"""

def configure(conf):
	"""
	Try to find a suitable C compiler or raise a :py:class:`waflib.Errors.ConfigurationError`.
	"""
	try: test_for_compiler = conf.options.check_c_compiler
	except AttributeError: conf.fatal("Add options(opt): opt.load('compiler_c')")
	for compiler in test_for_compiler.split():
		conf.env.stash()
		conf.start_msg('Checking for %r (c compiler)' % compiler)
		try:
			conf.load(compiler)
		except conf.errors.ConfigurationError as e:
			conf.env.revert()
			conf.end_msg(False)
			debug('compiler_c: %r' % e)
		else:
			if conf.env['CC']:
				conf.end_msg(conf.env.get_flat('CC'))
				conf.env['COMPILER_CC'] = compiler
				break
			conf.end_msg(False)
	else:
		conf.fatal('could not configure a c compiler!')

def options(opt):
	"""
	Restrict the compiler detection from the command-line::

		$ waf configure --check-c-compiler=gcc
	"""
	opt.load_special_tools('c_*.py', ban=['c_dumbpreproc.py'])
	global c_compiler
	build_platform = Utils.unversioned_sys_platform()
	possible_compiler_list = c_compiler[build_platform in c_compiler and build_platform or 'default']
	test_for_compiler = ' '.join(possible_compiler_list)
	cc_compiler_opts = opt.add_option_group("C Compiler Options")
	cc_compiler_opts.add_option('--check-c-compiler', default="%s" % test_for_compiler,
		help='On this platform (%s) the following C-Compiler will be checked by default: "%s"' % (build_platform, test_for_compiler),
		dest="check_c_compiler")
	for x in test_for_compiler.split():
		opt.load('%s' % x)

