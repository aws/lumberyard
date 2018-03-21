#!/usr/bin/env python
# encoding: utf-8
# Thomas Nagy 2008-2010

"""
MacOSX related tools
"""

import os, shutil, sys, platform
from waflib import TaskGen, Task, Build, Options, Utils, Errors
from waflib.TaskGen import taskgen_method, feature, after_method, before_method

app_info = '''
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist SYSTEM "file://localhost/System/Library/DTDs/PropertyList.dtd">
<plist version="0.9">
<dict>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleGetInfoString</key>
	<string>Created by Waf</string>
	<key>CFBundleSignature</key>
	<string>????</string>
	<key>NOTE</key>
	<string>THIS IS A GENERATED FILE, DO NOT MODIFY</string>
	<key>CFBundleExecutable</key>
	<string>%s</string>
</dict>
</plist>
'''
"""
plist template
"""

@feature('c', 'cxx')
def set_macosx_deployment_target(self):
	"""
	see WAF issue 285 and also and also http://trac.macports.org/ticket/17059
	"""
	if self.env['MACOSX_DEPLOYMENT_TARGET']:
		os.environ['MACOSX_DEPLOYMENT_TARGET'] = self.env['MACOSX_DEPLOYMENT_TARGET']
	elif 'MACOSX_DEPLOYMENT_TARGET' not in os.environ:
		if Utils.unversioned_sys_platform() == 'darwin':
			os.environ['MACOSX_DEPLOYMENT_TARGET'] = '.'.join(platform.mac_ver()[0].split('.')[:2])

@taskgen_method
def create_bundle_dirs(self, name, out):
	"""
	Create bundle folders, used by :py:func:`create_task_macplist` and :py:func:`create_task_macapp`
	"""
	bld = self.bld
	dir = out.parent.find_or_declare(name)
	dir.mkdir()
	macos = dir.find_or_declare(['Contents', 'MacOS'])
	macos.mkdir()
	return dir

def bundle_name_for_output(out):
	name = out.name
	k = name.rfind('.')
	if k >= 0:
		name = name[:k] + '.app'
	else:
		name = name + '.app'
	return name

@feature('cprogram', 'cxxprogram')
@after_method('add_rpath_update_tasks')
def create_task_macapp(self):
	"""
	To compile an executable into a Mac application (a .app), set its *mac_app* attribute::

		def build(bld):
			bld.shlib(source='a.c', target='foo', mac_app = True)

	To force *all* executables to be transformed into Mac applications::

		def build(bld):
			bld.env.MACAPP = True
			bld.shlib(source='a.c', target='foo')
	"""

	# don't excute this if the current platform is not darwin
	curr_platform, curr_configuration = self.bld.get_platform_and_configuration()
	if curr_platform.startswith("darwin_") is False:
		return

	if self.env['MACAPP'] or getattr(self, 'mac_app', False):
		out = self.link_task.outputs[0]
		name = bundle_name_for_output(out)

		current_platform = self.bld.env['PLATFORM']
		current_configuration = self.bld.env['CONFIGURATION']

		for target_node in self.bld.get_output_folders(current_platform, current_configuration, self):

			n1 = target_node.make_node([name, 'Contents', 'MacOS', out.name])

			self.apptask = self.create_macapp_task(out, n1)

			# Make sure that the executable is ready before the task is executed since
			# "update_to_use_rpath" task is modifying the executable.
			# This feels like a hack to find a previous task to gate the current task,
			# we should see if there are any better solutions.
			for task in self.tasks:
				if task.__class__.__name__ == "update_to_use_rpath":
					self.apptask.set_run_after(task)

			inst_to = getattr(self, 'install_path', '/Applications') + '/%s/Contents/MacOS/' % name
			self.bld.install_files(inst_to, n1, chmod=Utils.O755)

			if getattr(self, 'mac_resources', None):
				res_dir = n1.parent.parent.make_node('Resources')
				inst_to = getattr(self, 'install_path', '/Applications') + '/%s/Resources' % name
				for x in self.to_list(self.mac_resources):
					node = self.path.find_node(x)
					if not node:
						raise Errors.WafError('Missing mac_resource %r in %r' % (x, self))

					parent = node.parent
					if os.path.isdir(node.abspath()):
						nodes = node.ant_glob('**')
					else:
						nodes = [node]
					for node in nodes:
						rel = node.path_from(parent)
						output_node = res_dir.make_node(rel)
						self.create_macapp_task(node, output_node)
						self.bld.install_as(inst_to + '/%s' % rel, node)

			if getattr(self.bld, 'is_install', None):
				# disable the normal binary installation
				self.install_task.hasrun = Task.SKIP_ME

@feature('cprogram', 'cxxprogram')
@after_method('add_rpath_update_tasks')
def create_task_macplist(self):
	"""
	Create a :py:class:`waflib.Tools.c_osx.macplist` instance.
	"""

	# don't excute this if the current platform is not darwin
	curr_platform, curr_configuration = self.bld.get_platform_and_configuration()
	if curr_platform.startswith("darwin_") is False:
		return
	
	if  self.env['MACAPP'] or getattr(self, 'mac_app', False):
		out = self.link_task.outputs[0]
		name = bundle_name_for_output(out)

		current_platform = self.bld.env['PLATFORM']
		current_configuration = self.bld.env['CONFIGURATION']

		for target_node in self.bld.get_output_folders(current_platform, current_configuration, self):

			n1 = target_node.make_node([name, 'Contents', 'Info.plist'])

			self.plisttask = plisttask = self.create_task('macplist', [], n1)

			node = getattr(self, 'mac_plist', None)
			if node:
				if isinstance(node,str):
					plisttask.code = self.mac_plist
				else:
					plisttask.inputs.extend(node)
			else:
				plisttask.code = app_info % self.link_task.outputs[0].name

			inst_to = getattr(self, 'install_path', '/Applications') + '/%s/Contents/' % name
			self.bld.install_files(inst_to, n1)

@feature('cshlib', 'cxxshlib')
@before_method('apply_link', 'propagate_uselib_vars')
def apply_bundle(self):
	"""
	To make a bundled shared library (a ``.bundle``), set the *mac_bundle* attribute::

		def build(bld):
			bld.shlib(source='a.c', target='foo', mac_bundle = True)

	To force *all* executables to be transformed into bundles::

		def build(bld):
			bld.env.MACBUNDLE = True
			bld.shlib(source='a.c', target='foo')
	"""
	if self.env['MACBUNDLE'] or getattr(self, 'mac_bundle', False):
		self.env['LINKFLAGS_cshlib'] = self.env['LINKFLAGS_cxxshlib'] = [] # disable the '-dynamiclib' flag
		self.env['cshlib_PATTERN'] = self.env['cxxshlib_PATTERN'] = self.env['macbundle_PATTERN']
		use = self.use = self.to_list(getattr(self, 'use', []))
		if not 'MACBUNDLE' in use:
			use.append('MACBUNDLE')

app_dirs = ['Contents', 'Contents/MacOS', 'Contents/Resources']

class macplist(Task.Task):
	"""
	Create plist files
	"""
	color = 'PINK'
	ext_in = ['.bin']
	def run(self):
		if getattr(self, 'code', None):
			txt = self.code
		else:
			txt = self.inputs[0].read()
		self.outputs[0].write(txt)

