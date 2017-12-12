#! /usr/bin/env python
# encoding: utf-8

"""
Take a group of C++ files and compile them at once.

def options(opt):
	opt.load('compiler_cxx unity')
"""

import re
from waflib import Task, Options, Logs
from waflib.Tools import ccroot, cxx, c_preproc
from waflib.TaskGen import extension, taskgen_method

MAX_BATCH = 20

def options(opt):
	global MAX_BATCH
	opt.add_option('--batchsize', action='store', dest='batchsize', type='int', default=MAX_BATCH, help='batch size (0 for no batch)')

class unity(Task.Task):
	color = 'BLUE'
	scan = c_preproc.scan
	def run(self):
		lst = ['#include "%s"\n' % node.abspath() for node in self.inputs]
		txt = ''.join(lst)
		self.outputs[0].write(txt)

@taskgen_method
def batch_size(self):
	return Options.options.batchsize

@extension('.cpp', '.cc', '.cxx', '.C', '.c++')
def make_cxx_batch(self, node):
	cnt = self.batch_size()
	if cnt <= 1:
		tsk = self.create_compiled_task('cxx', node)
		return tsk

	try:
		self.cnt_cxx
	except AttributeError:
		self.cnt_cxx = 0

	x = getattr(self, 'master_cxx', None)
	if not x or len(x.inputs) >= cnt:
		x = self.master_cxx = self.create_task('unity')
		cxxnode = node.parent.find_or_declare('union_%s_%d_%d.cxx' % (self.idx, self.cnt_cxx, cnt))
		self.master_cxx.outputs = [cxxnode]
		self.cnt_cxx += 1
		self.create_compiled_task('cxx', cxxnode)
	x.inputs.append(node)

