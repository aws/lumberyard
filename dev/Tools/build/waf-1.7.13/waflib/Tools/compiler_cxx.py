#!/usr/bin/env python
# encoding: utf-8
# Matthias Jahn jahn dôt matthias ât freenet dôt de 2007 (pmarat)

"""
Try to detect a C++ compiler from the list of supported compilers (g++, msvc, etc)::

	def options(opt):
		opt.load('compiler_cxx')
	def configure(cnf):
		cnf.load('compiler_cxx')
	def build(bld):
		bld.program(source='main.cpp', target='app')

The compilers are associated to platforms in :py:attr:`waflib.Tools.compiler_cxx.cxx_compiler`. To register
a new C++ compiler named *cfoo* (assuming the tool ``waflib/extras/cfoo.py`` exists), use::

	def options(opt):
		opt.load('compiler_cxx')
	def configure(cnf):
		from waflib.Tools.compiler_cxx import cxx_compiler
		cxx_compiler['win32'] = ['cfoo', 'msvc', 'gcc']
		cnf.load('compiler_cxx')
	def build(bld):
		bld.program(source='main.c', target='app')

Not all compilers need to have a specific tool. For example, the clang compilers can be detected by the gcc tools when using::

	$ CXX=clang waf configure
"""


import os, sys, imp, types
from waflib.Tools import ccroot
from waflib import Utils, Configure
from waflib.Logs import debug

cxx_compiler = {
'win32':  ['msvc', 'g++'],
'cygwin': ['g++'],
'darwin': ['g++'],
'aix':    ['xlc++', 'g++'],
'linux':  ['g++', 'icpc'],
'sunos':  ['sunc++', 'g++'],
'irix':   ['g++'],
'hpux':   ['g++'],
'gnu':    ['g++'],
'java':   ['g++', 'msvc', 'icpc'],
'default': ['g++']
}
"""
Dict mapping the platform names to waf tools finding specific compilers::

	from waflib.Tools.compiler_cxx import cxx_compiler
	cxx_compiler['linux'] = ['gxx', 'icpc', 'suncxx']
"""


def configure(conf):
	"""
	Try to find a suitable C++ compiler or raise a :py:class:`waflib.Errors.ConfigurationError`.
	"""
	try: test_for_compiler = conf.options.check_cxx_compiler
	except AttributeError: conf.fatal("Add options(opt): opt.load('compiler_cxx')")

	for compiler in test_for_compiler.split():
		conf.env.stash()
		conf.start_msg('Checking for %r (c++ compiler)' % compiler)
		try:
			conf.load(compiler)
		except conf.errors.ConfigurationError as e:
			conf.env.revert()
			conf.end_msg(False)
			debug('compiler_cxx: %r' % e)
		else:
			if conf.env['CXX']:
				conf.end_msg(conf.env.get_flat('CXX'))
				conf.env['COMPILER_CXX'] = compiler
				break
			conf.end_msg(False)
	else:
		conf.fatal('could not configure a c++ compiler!')

def options(opt):
	"""
	Restrict the compiler detection from the command-line::

		$ waf configure --check-cxx-compiler=gxx
	"""
	opt.load_special_tools('cxx_*.py')
	global cxx_compiler
	build_platform = Utils.unversioned_sys_platform()
	possible_compiler_list = cxx_compiler[build_platform in cxx_compiler and build_platform or 'default']
	test_for_compiler = ' '.join(possible_compiler_list)
	cxx_compiler_opts = opt.add_option_group('C++ Compiler Options')
	cxx_compiler_opts.add_option('--check-cxx-compiler', default="%s" % test_for_compiler,
		help='On this platform (%s) the following C++ Compiler will be checked by default: "%s"' % (build_platform, test_for_compiler),
		dest="check_cxx_compiler")

	for x in test_for_compiler.split():
		opt.load('%s' % x)

