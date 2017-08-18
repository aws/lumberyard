#!/usr/bin/env python
# encoding: utf-8
#
# partially based on boost.py written by Gernot Vormayr
# written by Ruediger Sonderfeld <ruediger@c-plusplus.de>, 2008
# modified by Bjoern Michaelsen, 2008
# modified by Luca Fossati, 2008
# rewritten for waf 1.5.1, Thomas Nagy, 2008
# rewritten for waf 1.6.2, Sylvain Rouquette, 2011

'''

This is an extra tool, not bundled with the default waf binary.
To add the boost tool to the waf file:
$ ./waf-light --tools=compat15,boost
	or, if you have waf >= 1.6.2
$ ./waf update --files=boost

When using this tool, the wscript will look like:

	def options(opt):
		opt.load('compiler_cxx boost')

	def configure(conf):
		conf.load('compiler_cxx boost')
		conf.check_boost(lib='system filesystem')

	def build(bld):
		bld(source='main.cpp', target='app', use='BOOST')

Options are generated, in order to specify the location of boost includes/libraries.
The `check_boost` configuration function allows to specify the used boost libraries.
It can also provide default arguments to the --boost-static and --boost-mt command-line arguments.
Everything will be packaged together in a BOOST component that you can use.

When using MSVC, a lot of compilation flags need to match your BOOST build configuration:
 - you may have to add /EHsc to your CXXFLAGS or define boost::throw_exception if BOOST_NO_EXCEPTIONS is defined.
   Errors: C4530
 - boost libraries will try to be smart and use the (pretty but often not useful) auto-linking feature of MSVC
   So before calling `conf.check_boost` you might want to disabling by adding:
   	conf.env.DEFINES_BOOST += ['BOOST_ALL_NO_LIB']
   Errors: 
 - boost might also be compiled with /MT, which links the runtime statically.
   If you have problems with redefined symbols, 
		self.env['DEFINES_%s' % var] += ['BOOST_ALL_NO_LIB']
		self.env['CXXFLAGS_%s' % var] += ['/MD', '/EHsc']
Passing `--boost-linkage_autodetect` might help ensuring having a correct linkage in some basic cases.

'''

import sys
import re
from waflib import Utils, Logs, Errors
from waflib.Configure import conf

BOOST_LIBS = ['/usr/lib', '/usr/local/lib', '/opt/local/lib', '/sw/lib', '/lib']
BOOST_INCLUDES = ['/usr/include', '/usr/local/include', '/opt/local/include', '/sw/include']
BOOST_VERSION_FILE = 'boost/version.hpp'
BOOST_VERSION_CODE = '''
#include <iostream>
#include <boost/version.hpp>
int main() { std::cout << BOOST_LIB_VERSION << std::endl; }
'''

# toolsets from {boost_dir}/tools/build/v2/tools/common.jam
PLATFORM = Utils.unversioned_sys_platform()
detect_intel = lambda env: (PLATFORM == 'win32') and 'iw' or 'il'
detect_clang = lambda env: (PLATFORM == 'darwin') and 'clang-darwin' or 'clang'
detect_mingw = lambda env: (re.search('MinGW', env.CXX[0])) and 'mgw' or 'gcc'
BOOST_TOOLSETS = {
	'borland':  'bcb',
	'clang':	detect_clang,
	'como':	 'como',
	'cw':	   'cw',
	'darwin':   'xgcc',
	'edg':	  'edg',
	'g++':	  detect_mingw,
	'gcc':	  detect_mingw,
	'icpc':	 detect_intel,
	'intel':	detect_intel,
	'kcc':	  'kcc',
	'kylix':	'bck',
	'mipspro':  'mp',
	'mingw':	'mgw',
	'msvc':	 'vc',
	'qcc':	  'qcc',
	'sun':	  'sw',
	'sunc++':   'sw',
	'tru64cxx': 'tru',
	'vacpp':	'xlc'
}


def options(opt):
	opt.add_option('--boost-includes', type='string',
				   default='', dest='boost_includes',
				   help='''path to the boost includes root (~boost root)
				   e.g. /path/to/boost_1_47_0''')
	opt.add_option('--boost-libs', type='string',
				   default='', dest='boost_libs',
				   help='''path to the directory where the boost libs are
				   e.g. /path/to/boost_1_47_0/stage/lib''')
	opt.add_option('--boost-static', action='store_true',
				   default=False, dest='boost_static',
				   help='link with static boost libraries (.lib/.a)')
	opt.add_option('--boost-mt', action='store_true',
				   default=False, dest='boost_mt',
				   help='select multi-threaded libraries')
	opt.add_option('--boost-abi', type='string', default='', dest='boost_abi',
				   help='''select libraries with tags (dgsyp, d for debug),
				   see doc Boost, Getting Started, chapter 6.1''')
	opt.add_option('--boost-linkage_autodetect', action="store_true", dest='boost_linkage_autodetect',
				   help="auto-detect boost linkage options (don't get used to it / might break other stuff)")
	opt.add_option('--boost-toolset', type='string',
				   default='', dest='boost_toolset',
				   help='force a toolset e.g. msvc, vc90, \
						gcc, mingw, mgw45 (default: auto)')
	py_version = '%d%d' % (sys.version_info[0], sys.version_info[1])
	opt.add_option('--boost-python', type='string',
				   default=py_version, dest='boost_python',
				   help='select the lib python with this version \
						(default: %s)' % py_version)


@conf
def __boost_get_version_file(self, d):
	dnode = self.root.find_dir(d)
	if dnode:
		return dnode.find_node(BOOST_VERSION_FILE)
	return None

@conf
def boost_get_version(self, d):
	"""silently retrieve the boost version number"""
	node = self.__boost_get_version_file(d)
	if node:
		try:
			txt = node.read()
		except (OSError, IOError):
			Logs.error("Could not read the file %r" % node.abspath())
		else:
			re_but = re.compile('^#define\\s+BOOST_LIB_VERSION\\s+"(.*)"', re.M)
			m = re_but.search(txt)
			if m:
				return m.group(1)
	return self.check_cxx(fragment=BOOST_VERSION_CODE, includes=[d], execute=True, define_ret=True)

@conf
def boost_get_includes(self, *k, **kw):
	includes = k and k[0] or kw.get('includes', None)
	if includes and self.__boost_get_version_file(includes):
		return includes
	for d in Utils.to_list(self.environ.get('INCLUDE', '')) + BOOST_INCLUDES:
		if self.__boost_get_version_file(d):
			return d
	if includes:
		self.end_msg('headers not found in %s' % includes)
		self.fatal('The configuration failed')
	else:
		self.end_msg('headers not found, please provide a --boost-includes argument (see help)')
		self.fatal('The configuration failed')


@conf
def boost_get_toolset(self, cc):
	toolset = cc
	if not cc:
		build_platform = Utils.unversioned_sys_platform()
		if build_platform in BOOST_TOOLSETS:
			cc = build_platform
		else:
			cc = self.env.CXX_NAME
	if cc in BOOST_TOOLSETS:
		toolset = BOOST_TOOLSETS[cc]
	return isinstance(toolset, str) and toolset or toolset(self.env)


@conf
def __boost_get_libs_path(self, *k, **kw):
	''' return the lib path and all the files in it '''
	if 'files' in kw:
		return self.root.find_dir('.'), Utils.to_list(kw['files'])
	libs = k and k[0] or kw.get('libs', None)
	if libs:
		path = self.root.find_dir(libs)
		files = path.ant_glob('*boost_*')
	if not libs or not files:
		for d in Utils.to_list(self.environ.get('LIB', [])) + BOOST_LIBS:
			path = self.root.find_dir(d)
			if path:
				files = path.ant_glob('*boost_*')
				if files:
					break
			path = self.root.find_dir(d + '64')
			if path:
				files = path.ant_glob('*boost_*')
				if files:
					break
	if not path:
		if libs:
			self.end_msg('libs not found in %s' % libs)
			self.fatal('The configuration failed')
		else:
			self.end_msg('libs not found, please provide a --boost-libs argument (see help)')
			self.fatal('The configuration failed')

	self.to_log('Found the boost path in %r with the libraries:' % path)
	for x in files:
		self.to_log('    %r' % x)
	return path, files

@conf
def boost_get_libs(self, *k, **kw):
	'''
	return the lib path and the required libs
	according to the parameters
	'''
	path, files = self.__boost_get_libs_path(**kw)
	t = []
	if kw.get('mt', False):
		t.append('mt')
	if kw.get('abi', None):
		t.append(kw['abi'])
	tags = t and '(-%s)+' % '-'.join(t) or ''
	toolset = self.boost_get_toolset(kw.get('toolset', ''))
	toolset_pat = '(-%s[0-9]{0,3})+' % toolset
	version = '(-%s)+' % self.env.BOOST_VERSION

	def find_lib(re_lib, files):
		for file in files:
			if re_lib.search(file.name):
				self.to_log('Found boost lib %s' % file)
				return file
		return None

	def format_lib_name(name):
		if name.startswith('lib') and self.env.CC_NAME != 'msvc':
			name = name[3:]
		return name[:name.rfind('.')]

	libs = []
	for lib in Utils.to_list(k and k[0] or kw.get('lib', None)):
		py = (lib == 'python') and '(-py%s)+' % kw['python'] or ''
		# Trying libraries, from most strict match to least one
		for pattern in ['boost_%s%s%s%s%s' % (lib, toolset_pat, tags, py, version),
						'boost_%s%s%s%s' % (lib, tags, py, version),
						'boost_%s%s%s' % (lib, tags, version),
						# Give up trying to find the right version
						'boost_%s%s%s%s' % (lib, toolset_pat, tags, py),
						'boost_%s%s%s' % (lib, tags, py),
						'boost_%s%s' % (lib, tags)]:
			self.to_log('Trying pattern %s' % pattern)
			file = find_lib(re.compile(pattern), files)
			if file:
				libs.append(format_lib_name(file.name))
				break
		else:
			self.end_msg('lib %s not found in %s' % (lib, path.abspath()))
			self.fatal('The configuration failed')

	return path.abspath(), libs


@conf
def check_boost(self, *k, **kw):
	"""
	Initialize boost libraries to be used.

	Keywords: you can pass the same parameters as with the command line (without "--boost-").
	Note that the command line has the priority, and should preferably be used.
	"""
	if not self.env['CXX']:
		self.fatal('load a c++ compiler first, conf.load("compiler_cxx")')

	params = {'lib': k and k[0] or kw.get('lib', None)}
	for key, value in self.options.__dict__.items():
		if not key.startswith('boost_'):
			continue
		key = key[len('boost_'):]
		params[key] = value and value or kw.get(key, '')

	var = kw.get('uselib_store', 'BOOST')

	self.start_msg('Checking boost includes')
	self.env['INCLUDES_%s' % var] = inc = self.boost_get_includes(**params)
	self.env.BOOST_VERSION = self.boost_get_version(inc)
	self.end_msg(self.env.BOOST_VERSION)
	if Logs.verbose:
		Logs.pprint('CYAN', '	path : %s' % self.env['INCLUDES_%s' % var])

	if not params['lib']:
		return
	self.start_msg('Checking boost libs')
	suffix = params.get('static', None) and 'ST' or ''
	path, libs = self.boost_get_libs(**params)
	self.env['%sLIBPATH_%s' % (suffix, var)] = [path]
	self.env['%sLIB_%s' % (suffix, var)] = libs
	self.end_msg('ok')
	if Logs.verbose:
		Logs.pprint('CYAN', '	path : %s' % path)
		Logs.pprint('CYAN', '	libs : %s' % libs)


	def try_link():
		if 'system' in params['lib']:
			self.check_cxx(
			 fragment="\n".join([
			  '#include <boost/system/error_code.hpp>',
			  'int main() { boost::system::error_code c; }',
			 ]),
			 use=var,
			 execute=False,
			)
		if 'thread' in params['lib']:
			self.check_cxx(
			 fragment="\n".join([
			  '#include <boost/thread.hpp>',
			  'int main() { boost::thread t; }',
			 ]),
			 use=var,
			 execute=False,
			)

	if params.get('linkage_autodetect', False):
		self.start_msg("Attempting to detect boost linkage flags")
		toolset = self.boost_get_toolset(kw.get('toolset', ''))
		if toolset in ['vc']:
			# disable auto-linking feature, causing error LNK1181
			# because the code wants to be linked against
			self.env['DEFINES_%s' % var] += ['BOOST_ALL_NO_LIB']

			# if no dlls are present, we guess the .lib files are not stubs
			has_dlls = False
			for x in Utils.listdir(path):
				if x.endswith(self.env.cxxshlib_PATTERN % ''):
					has_dlls = True
					break
			if not has_dlls:
				self.env['STLIBPATH_%s' % var] = [path]
				self.env['STLIB_%s' % var] = libs
				del self.env['LIB_%s' % var]
				del self.env['LIBPATH_%s' % var]

			# we attempt to play with some known-to-work CXXFLAGS combinations
			for cxxflags in (['/MD', '/EHsc'], []):
				self.env.stash()
				self.env["CXXFLAGS_%s" % var] += cxxflags
				try:
					try_link()
					self.end_msg("ok: winning cxxflags combination: %s" % (self.env["CXXFLAGS_%s" % var]))
					e = None
					break
				except Errors.ConfigurationError as exc:
					self.env.revert()
					e = exc

			if e is not None:
				self.end_msg("Could not auto-detect boost linking flags combination, you may report it to boost.py author", ex=e)
				self.fatal('The configuration failed')
		else:
			self.end_msg("Boost linkage flags auto-detection not implemented (needed ?) for this toolchain")
			self.fatal('The configuration failed')
	else:
		self.start_msg('Checking for boost linkage')
		try:
			try_link()
		except Errors.ConfigurationError as e:
			self.end_msg("Could not link against boost libraries using supplied options")
			self.fatal('The configuration failed')
		self.end_msg('ok')

