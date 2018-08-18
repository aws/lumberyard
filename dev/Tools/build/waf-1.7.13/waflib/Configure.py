#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy, 2005-2010 (ita)

"""
Configuration system

A :py:class:`waflib.Configure.ConfigurationContext` instance is created when ``waf configure`` is called, it is used to:

* create data dictionaries (ConfigSet instances)
* store the list of modules to import
* hold configuration routines such as ``find_program``, etc
"""

import os, shlex, sys, time
from waflib import ConfigSet, Utils, Options, Logs, Context, Build, Errors

try:
	from urllib import request
except ImportError:
	from urllib import urlopen
else:
	urlopen = request.urlopen

BREAK    = 'break'
"""In case of a configuration error, break"""

CONTINUE = 'continue'
"""In case of a configuration error, continue"""

WAF_CONFIG_LOG = 'config.log'
"""Name of the configuration log file"""

autoconfig = False
"""Execute the configuration automatically"""

conf_template = '''# project %(app)s configured on %(now)s by
# waf %(wafver)s (abi %(abi)s, python %(pyver)x on %(systype)s)
# using %(args)s
#'''

def download_check(node):
	"""
	Hook to check for the tools which are downloaded. Replace with your function if necessary.
	"""
	pass

def download_tool(tool, force=False, ctx=None):
	"""
	Download a Waf tool from the remote repository defined in :py:const:`waflib.Context.remote_repo`::

		$ waf configure --download
	"""
	for x in Utils.to_list(Context.remote_repo):
		for sub in Utils.to_list(Context.remote_locs):
			url = '/'.join((x, sub, tool + '.py'))
			try:
				web = urlopen(url)
				try:
					if web.getcode() != 200:
						continue
				except AttributeError:
					pass
			except Exception:
				# on python3 urlopen throws an exception
				# python 2.3 does not have getcode and throws an exception to fail
				continue
			else:
				tmp = ctx.root.make_node(os.sep.join((Context.waf_dir, 'waflib', 'extras', tool + '.py')))
				tmp.write(web.read(), 'wb')
				Logs.warn('Downloaded %s from %s' % (tool, url))
				download_check(tmp)
				try:
					module = Context.load_tool(tool)
				except Exception:
					Logs.warn('The tool %s from %s is unusable' % (tool, url))
					try:
						tmp.delete()
					except Exception:
						pass
					continue
				return module
	raise Errors.WafError('Could not load the Waf tool')

class ConfigurationContext(Context.Context):
	'''configures the project'''

	cmd = 'configure'

	error_handlers = []
	"""
	Additional functions to handle configuration errors
	"""

	def __init__(self, **kw):
		super(ConfigurationContext, self).__init__(**kw)
		self.environ = dict(os.environ)
		self.all_envs = {}

		self.top_dir = None
		self.out_dir = None
		self.lock_dir = None

		self.tools = [] # tools loaded in the configuration, and that will be loaded when building

		self.hash = 0
		self.files = []

		self.tool_cache = []

		self.setenv('')

	def setenv(self, name, env=None):
		"""
		Set a new config set for conf.env. If a config set of that name already exists,
		recall it without modification.

		The name is the filename prefix to save to ``c4che/NAME_cache.py``, and it
		is also used as *variants* by the build commands.
		Though related to variants, whatever kind of data may be stored in the config set::

			def configure(cfg):
				cfg.env.ONE = 1
				cfg.setenv('foo')
				cfg.env.ONE = 2

			def build(bld):
				2 == bld.env_of_name('foo').ONE

		:param name: name of the configuration set
		:type name: string
		:param env: ConfigSet to copy, or an empty ConfigSet is created
		:type env: :py:class:`waflib.ConfigSet.ConfigSet`
		"""
		if name not in self.all_envs or env:
			if not env:
				env = ConfigSet.ConfigSet()
				self.prepare_env(env)
			else:
				env = env.derive()
			self.all_envs[name] = env
		self.variant = name

	def get_env(self):
		"""Getter for the env property"""
		return self.all_envs[self.variant]
	def set_env(self, val):
		"""Setter for the env property"""
		self.all_envs[self.variant] = val

	env = property(get_env, set_env)

	def init_dirs(self):
		"""
		Initialize the project directory and the build directory
		"""

		top = self.top_dir
		if not top:
			top = Options.options.top
		if not top:
			top = getattr(Context.g_module, Context.TOP, None)
		if not top:
			top = self.path.abspath()
		top = os.path.abspath(top)

		self.srcnode = (os.path.isabs(top) and self.root or self.path).find_dir(top)
		assert(self.srcnode)

		out = self.out_dir
		if not out:
			out = Options.options.out
		if not out:
			out = getattr(Context.g_module, Context.OUT, None)
		if not out:
			out = Options.lockfile.replace('.lock-waf_%s_' % sys.platform, '').replace('.lock-waf', '')

		self.bldnode = (os.path.isabs(out) and self.root or self.path).make_node(out)
		self.bldnode.mkdir()

		if not os.path.isdir(self.bldnode.abspath()):
			conf.fatal('Could not create the build directory %s' % self.bldnode.abspath())

	def execute(self):
		"""
		See :py:func:`waflib.Context.Context.execute`
		"""		
		self.init_dirs()
		Logs.info("[WAF] Executing 'configure'")
		
		self.cachedir = self.bldnode.make_node(Build.CACHE_DIR)
		self.cachedir.mkdir()

		path = os.path.join(self.bldnode.abspath(), WAF_CONFIG_LOG)
		self.logger = Logs.make_logger(path, 'cfg')

		app = getattr(Context.g_module, 'APPNAME', '')
		if app:
			ver = getattr(Context.g_module, 'VERSION', '')
			if ver:
				app = "%s (%s)" % (app, ver)

		now = time.ctime()
		pyver = sys.hexversion
		systype = sys.platform
		args = " ".join(sys.argv)
		wafver = Context.WAFVERSION
		abi = Context.ABI
		self.to_log(conf_template % vars())

		if id(self.srcnode) == id(self.bldnode):
			Logs.warn('Setting top == out (remember to use "update_outputs")')
		elif id(self.path) != id(self.srcnode):
			if self.srcnode.is_child_of(self.path):
				Logs.warn('Are you certain that you do not want to set top="." ?')

		super(ConfigurationContext, self).execute()

		self.store()

		Context.top_dir = self.srcnode.abspath()
		Context.out_dir = self.bldnode.abspath()
		
		# import waf branch spec
		branch_spec_globals = Context.load_branch_spec(Context.top_dir)
					
		Context.lock_dir = Context.run_dir + os.sep + branch_spec_globals['BINTEMP_FOLDER']

		# this will write a configure lock so that subsequent builds will
		# consider the current path as the root directory (see prepare_impl).
		# to remove: use 'waf distclean'
		env = ConfigSet.ConfigSet()
		env['argv'] = sys.argv
		env['options'] = Options.options.__dict__

		env.run_dir = Context.run_dir
		env.top_dir = Context.top_dir
		env.out_dir = Context.out_dir
		env.lock_dir = Context.lock_dir

		# Add lmbr_waf.bat or lmbr_waf for dependency tracking
        ###############################################################################
		waf_command = os.path.basename(sys.executable)
		if waf_command.lower().startswith('python'):
			waf_executable = self.engine_node.make_node('./Tools/build/waf-1.7.13/lmbr_waf')
		else:
			waf_executable = self.path.make_node(waf_command)

		self.hash = hash((self.hash, waf_executable.read('rb')))
		self.files.append(os.path.normpath(waf_executable.abspath()))

		# conf.hash & conf.files hold wscript files paths and hash
		# (used only by Configure.autoconfig)
		env['hash'] = self.hash
		env['files'] = self.files
		env['environ'] = dict(self.environ)
			
		env.store(Context.lock_dir + os.sep + Options.lockfile)

	def prepare_env(self, env):
		"""
		Insert *PREFIX*, *BINDIR* and *LIBDIR* values into ``env``

		:type env: :py:class:`waflib.ConfigSet.ConfigSet`
		:param env: a ConfigSet, usually ``conf.env``
		"""
		if not env.PREFIX:
			if Options.options.prefix or Utils.is_win32:
				env.PREFIX = os.path.abspath(os.path.expanduser(Options.options.prefix))
			else:
				env.PREFIX = ''
		if not env.BINDIR:
			env.BINDIR = Utils.subst_vars('${PREFIX}/bin', env)
		if not env.LIBDIR:
			env.LIBDIR = Utils.subst_vars('${PREFIX}/lib', env)

	def store(self):
		"""Save the config results into the cache file"""
		n = self.cachedir.make_node('build.config.py')
		n.write('version = 0x%x\ntools = %r\n' % (Context.HEXVERSION, self.tools))

		if not self.all_envs:
			self.fatal('nothing to store in the configuration context!')

		for key in self.all_envs:
			tmpenv = self.all_envs[key]
			tmpenv.store(os.path.join(self.cachedir.abspath(), key + Build.CACHE_SUFFIX))

	def load(self, input, tooldir=None, funs=None, download=True):
		"""
		Load Waf tools, which will be imported whenever a build is started.

		:param input: waf tools to import
		:type input: list of string
		:param tooldir: paths for the imports
		:type tooldir: list of string
		:param funs: functions to execute from the waf tools
		:type funs: list of string
		:param download: whether to download the tool from the waf repository
		:type download: bool
		"""

		tools = Utils.to_list(input)
		if tooldir: 
			tooldir = Utils.to_list(tooldir)
			# Assume that whenever we specify a tooldir, we want to track those files
			if os.path.isabs(tooldir[0]):
				lmbr_waf_lib = self.root.make_node(tooldir).make_node(input + '.py')
			else:
				lmbr_waf_lib = self.path.make_node(tooldir).make_node(input + '.py')
			self.hash = hash((self.hash, lmbr_waf_lib.read('rb')))
			self.files.append(os.path.normpath(lmbr_waf_lib.abspath()))			
		for tool in tools:
			# avoid loading the same tool more than once with the same functions
			# used by composite projects

			mag = (tool, id(self.env), funs)
			if mag in self.tool_cache:
				self.to_log('(tool %s is already loaded, skipping)' % tool)
				continue
			self.tool_cache.append(mag)

			module = None
			try:
				module = Context.load_tool(tool, tooldir)
			except ImportError as e:
				if Options.options.download:
					module = download_tool(tool, ctx=self)
					if not module:
						self.fatal('Could not load the Waf tool %r or download a suitable replacement from the repository (sys.path %r)\n%s' % (tool, sys.path, e))
				else:
					self.fatal('Could not load the Waf tool %r from %r (try the --download option?):\n%s' % (tool, sys.path, e))
			except Exception as e:
				self.to_log('imp %r (%r & %r)' % (tool, tooldir, funs))
				self.to_log(Utils.ex_stack())
				raise

			if funs is not None:
				self.eval_rules(funs)
			else:
				func = getattr(module, 'configure', None)
				if func:
					if type(func) is type(Utils.readf): func(self)
					else: self.eval_rules(func)

			self.tools.append({'tool':tool, 'tooldir':tooldir, 'funs':funs})

	def post_recurse(self, node):
		"""
		Records the path and a hash of the scripts visited, see :py:meth:`waflib.Context.Context.post_recurse`

		:param node: script
		:type node: :py:class:`waflib.Node.Node`
		"""
		super(ConfigurationContext, self).post_recurse(node)
		self.hash = hash((self.hash, node.read('rb')))
		self.files.append(node.abspath())
		
		if hasattr(self, 'addional_files_to_track'):			
			for file_node in self.addional_files_to_track:
				#print 'found addional_files_to_track ', file_node
				self.hash = hash((self.hash, file_node.read('rb')))
				self.files.append(file_node.abspath())
			self.addional_files_to_track = []
			
	def eval_rules(self, rules):
		"""
		Execute the configuration tests. The method :py:meth:`waflib.Configure.ConfigurationContext.err_handler`
		is used to process the eventual exceptions

		:param rules: list of configuration method names
		:type rules: list of string
		"""
		self.rules = Utils.to_list(rules)
		for x in self.rules:
			f = getattr(self, x)
			if not f: self.fatal("No such method '%s'." % x)
			try:
				f()
			except Exception as e:
				ret = self.err_handler(x, e)
				if ret == BREAK:
					break
				elif ret == CONTINUE:
					continue
				else:
					raise

	def err_handler(self, fun, error):
		"""
		Error handler for the configuration tests, the default is to let the exception raise

		:param fun: configuration test
		:type fun: method
		:param error: exception
		:type error: exception
		"""
		pass

def conf(f):
	"""
	Decorator: attach new configuration functions to :py:class:`waflib.Build.BuildContext` and
	:py:class:`waflib.Configure.ConfigurationContext`. The methods bound will accept a parameter
	named 'mandatory' to disable the configuration errors::

		def configure(conf):
			conf.find_program('abc', mandatory=False)

	:param f: method to bind
	:type f: function
	"""
	def fun(*k, **kw):
		mandatory = True
		if 'mandatory' in kw:
			mandatory = kw['mandatory']
			del kw['mandatory']

		try:
			return f(*k, **kw)
		except Errors.ConfigurationError:
			if mandatory:
				raise

	setattr(Options.OptionsContext, f.__name__, fun)
	setattr(ConfigurationContext, f.__name__, fun)
	setattr(Build.BuildContext, f.__name__, fun)
	return f

@conf
def add_os_flags(self, var, dest=None):
	"""
	Import operating system environment values into ``conf.env`` dict::

		def configure(conf):
			conf.add_os_flags('CFLAGS')

	:param var: variable to use
	:type var: string
	:param dest: destination variable, by default the same as var
	:type dest: string
	"""
	# do not use 'get' to make certain the variable is not defined
	try: self.env.append_value(dest or var, shlex.split(self.environ[var]))
	except KeyError: pass

@conf
def cmd_to_list(self, cmd):
	"""
	Detect if a command is written in pseudo shell like ``ccache g++`` and return a list.

	:param cmd: command
	:type cmd: a string or a list of string
	"""
	if isinstance(cmd, str) and cmd.find(' '):
		try:
			os.stat(cmd)
		except OSError:
			return shlex.split(cmd)
		else:
			return [cmd]
	return cmd

@conf
def check_waf_version(self, mini='1.6.99', maxi='1.8.0'):
	"""
	Raise a Configuration error if the Waf version does not strictly match the given bounds::

		conf.check_waf_version(mini='1.7.0', maxi='1.8.0')

	:type  mini: number, tuple or string
	:param mini: Minimum required version
	:type  maxi: number, tuple or string
	:param maxi: Maximum allowed version
	"""
	self.start_msg('Checking for waf version in %s-%s' % (str(mini), str(maxi)))
	ver = Context.HEXVERSION
	if Utils.num2ver(mini) > ver:
		self.fatal('waf version should be at least %r (%r found)' % (Utils.num2ver(mini), ver))

	if Utils.num2ver(maxi) < ver:
		self.fatal('waf version should be at most %r (%r found)' % (Utils.num2ver(maxi), ver))
	self.end_msg('ok')

@conf
def find_file(self, filename, path_list=[]):
	"""
	Find a file in a list of paths

	:param filename: name of the file to search for
	:param path_list: list of directories to search
	:return: the first occurrence filename or '' if filename could not be found
	"""
	for n in Utils.to_list(filename):
		for d in Utils.to_list(path_list):
			p = os.path.join(d, n)
			if os.path.exists(p):
				return p
	self.fatal('Could not find %r' % filename)

@conf
def find_program(self, filename, **kw):
	"""
	Search for a program on the operating system

	When var is used, you may set os.environ[var] to help find a specific program version, for example::

		$ VALAC=/usr/bin/valac_test waf configure

	:param path_list: paths to use for searching
	:type param_list: list of string
	:param var: store the result to conf.env[var], by default use filename.upper()
	:type var: string
	:param ext: list of extensions for the binary (do not add an extension for portability)
	:type ext: list of string
	"""

	exts = kw.get('exts', Utils.is_win32 and '.exe,.com,.bat,.cmd' or ',.sh,.pl,.py')

	environ = kw.get('environ', os.environ)

	ret = ''
	filename = Utils.to_list(filename)

	var = kw.get('var', '')
	if not var:
		var = filename[0].upper()

	if self.env[var]:
		ret = self.env[var]
	elif var in environ:
		ret = environ[var]

	path_list = kw.get('path_list', '')
	if not ret:
		if path_list:
			path_list = Utils.to_list(path_list)
		else:
			path_list = environ.get('PATH', '').split(os.pathsep)

		if not isinstance(filename, list):
			filename = [filename]

		for a in exts.split(','):
			if ret:
				break
			for b in filename:
				if ret:
					break
				for c in path_list:
					if ret:
						break
					x = os.path.expanduser(os.path.join(c, b + a))
					if os.path.isfile(x):
						ret = x

	if not ret and Utils.winreg:
		ret = Utils.get_registry_app_path(Utils.winreg.HKEY_CURRENT_USER, filename)
	if not ret and Utils.winreg:
		ret = Utils.get_registry_app_path(Utils.winreg.HKEY_LOCAL_MACHINE, filename)

	if not kw.get('silent_output'):
		self.msg('Checking for program ' + ','.join(filename), ret or False)
		self.to_log('find program=%r paths=%r var=%r -> %r' % (filename, path_list, var, ret))

	if not ret:
		self.fatal(kw.get('errmsg', '') or 'Could not find the program %s' % ','.join(filename))

	if var:
		self.env[var] = ret
	return ret


@conf
def find_perl_program(self, filename, path_list=[], var=None, environ=None, exts=''):
	"""
	Search for a perl program on the operating system

	:param filename: file to search for
	:type filename: string
	:param path_list: list of paths to look into
	:type path_list: list of string
	:param var: store the results into *conf.env.var*
	:type var: string
	:param environ: operating system environment to pass to :py:func:`waflib.Configure.find_program`
	:type environ: dict
	:param exts: extensions given to :py:func:`waflib.Configure.find_program`
	:type exts: list
	"""

	try:
		app = self.find_program(filename, path_list=path_list, var=var, environ=environ, exts=exts)
	except Exception:
		self.find_program('perl', var='PERL')
		app = self.find_file(filename, os.environ['PATH'].split(os.pathsep))
		if not app:
			raise
		if var:
			self.env[var] = Utils.to_list(self.env['PERL']) + [app]
	self.msg('Checking for %r' % filename, app)

