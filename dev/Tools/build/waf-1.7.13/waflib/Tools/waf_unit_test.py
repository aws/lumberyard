#!/usr/bin/env python
# encoding: utf-8
# Carlos Rafael Giani, 2006
# Thomas Nagy, 2010

"""
Unit testing system for C/C++/D providing test execution:

* in parallel, by using ``waf -j``
* partial (only the tests that have changed) or full (by using ``waf --alltests``)

The tests are declared by adding the **test** feature to programs::

	def options(opt):
		opt.load('compiler_cxx waf_unit_test')
	def configure(conf):
		conf.load('compiler_cxx waf_unit_test')
	def build(bld):
		bld(features='cxx cxxprogram test', source='main.cpp', target='app')
		# or
		bld.program(features='test', source='main2.cpp', target='app2')

When the build is executed, the program 'test' will be built and executed without arguments.
The success/failure is detected by looking at the return code. The status and the standard output/error
are stored on the build context.

The results can be displayed by registering a callback function. Here is how to call
the predefined callback::

	def build(bld):
		bld(features='cxx cxxprogram test', source='main.c', target='app')
		from waflib.Tools import waf_unit_test
		bld.add_post_fun(waf_unit_test.summary)
"""

import os, sys
from waflib.TaskGen import feature, after_method
from waflib import Utils, Task, Logs, Options
testlock = Utils.threading.Lock()

@feature('test')
@after_method('apply_link')
def make_test(self):
	"""Create the unit test task. There can be only one unit test task by task generator."""
	if getattr(self, 'link_task', None):
		self.create_task('utest', self.link_task.outputs)

class utest(Task.Task):
	"""
	Execute a unit test
	"""
	color = 'PINK'
	after = ['vnum', 'inst']
	vars = []
	def runnable_status(self):
		"""
		Always execute the task if `waf --alltests` was used or no
                tests if ``waf --notests`` was used
		"""
                if getattr(Options.options, 'no_tests', False):
                        return Task.SKIP_ME

		ret = super(utest, self).runnable_status()
		if ret == Task.SKIP_ME:
			if getattr(Options.options, 'all_tests', False):
				return Task.RUN_ME
		return ret

	def run(self):
		"""
		Execute the test. The execution is always successful, but the results
		are stored on ``self.generator.bld.utest_results`` for postprocessing.
		"""

		filename = self.inputs[0].abspath()
		self.ut_exec = getattr(self.generator, 'ut_exec', [filename])
		if getattr(self.generator, 'ut_fun', None):
			# FIXME waf 1.8 - add a return statement here?
			self.generator.ut_fun(self)

		try:
			fu = getattr(self.generator.bld, 'all_test_paths')
		except AttributeError:
			# this operation may be performed by at most #maxjobs
			fu = os.environ.copy()

			lst = []
			for g in self.generator.bld.groups:
				for tg in g:
					if getattr(tg, 'link_task', None):
						s = tg.link_task.outputs[0].parent.abspath()
						if s not in lst:
							lst.append(s)

			def add_path(dct, path, var):
				dct[var] = os.pathsep.join(Utils.to_list(path) + [os.environ.get(var, '')])

			if Utils.is_win32:
				add_path(fu, lst, 'PATH')
			elif Utils.unversioned_sys_platform() == 'darwin':
				add_path(fu, lst, 'DYLD_LIBRARY_PATH')
				add_path(fu, lst, 'LD_LIBRARY_PATH')
			else:
				add_path(fu, lst, 'LD_LIBRARY_PATH')
			self.generator.bld.all_test_paths = fu


		cwd = getattr(self.generator, 'ut_cwd', '') or self.inputs[0].parent.abspath()

                testcmd = getattr(Options.options, 'testcmd', False)
                if testcmd:
                        self.ut_exec = (testcmd % self.ut_exec[0]).split(' ')

		proc = Utils.subprocess.Popen(self.ut_exec, cwd=cwd, env=fu, stderr=Utils.subprocess.PIPE, stdout=Utils.subprocess.PIPE)
		(stdout, stderr) = proc.communicate()

		tup = (filename, proc.returncode, stdout, stderr)
		self.generator.utest_result = tup

		testlock.acquire()
		try:
			bld = self.generator.bld
			Logs.debug("ut: %r", tup)
			try:
				bld.utest_results.append(tup)
			except AttributeError:
				bld.utest_results = [tup]
		finally:
			testlock.release()

def summary(bld):
	"""
	Display an execution summary::

		def build(bld):
			bld(features='cxx cxxprogram test', source='main.c', target='app')
			from waflib.Tools import waf_unit_test
			bld.add_post_fun(waf_unit_test.summary)
	"""
	lst = getattr(bld, 'utest_results', [])
	if lst:
		Logs.pprint('CYAN', 'execution summary')

		total = len(lst)
		tfail = len([x for x in lst if x[1]])

		Logs.pprint('CYAN', '  tests that pass %d/%d' % (total-tfail, total))
		for (f, code, out, err) in lst:
			if not code:
				Logs.pprint('CYAN', '    %s' % f)

		Logs.pprint('CYAN', '  tests that fail %d/%d' % (tfail, total))
		for (f, code, out, err) in lst:
			if code:
				Logs.pprint('CYAN', '    %s' % f)

def set_exit_code(bld):
        """
        If any of the tests fail waf will exit with that exit code.
        This is useful if you have an automated build system which need
        to report on errors from the tests.
        You may use it like this:

		def build(bld):
			bld(features='cxx cxxprogram test', source='main.c', target='app')
			from waflib.Tools import waf_unit_test
			bld.add_post_fun(waf_unit_test.set_exit_code)
        """
        lst = getattr(bld, 'utest_results', [])
        for (f, code, out, err) in lst:
                if code:
                        msg = []
                        if out:
                                msg.append('stdout:%s%s' % (os.linesep, out.decode('utf-8')))
                        if err:
                                msg.append('stderr:%s%s' % (os.linesep, err.decode('utf-8')))
                        bld.fatal(os.linesep.join(msg))


def options(opt):
	"""
	Provide the ``--alltests``, ``--notests`` and ``--testcmd`` command-line options.
	"""
        opt.add_option('--notests', action='store_true', default=False, help='Exec no unit tests', dest='no_tests')
	opt.add_option('--alltests', action='store_true', default=False, help='Exec all unit tests', dest='all_tests')
        opt.add_option('--testcmd', action='store', default=False,
                       help = 'Run the unit tests using the test-cmd string'
                              ' example "--test-cmd="valgrind --error-exitcode=1'
                              ' %s" to run under valgrind', dest='testcmd')

