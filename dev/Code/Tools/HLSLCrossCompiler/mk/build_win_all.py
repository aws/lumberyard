from build import build
import os

configuration = 'release'

make_dir = os.path.dirname(os.path.realpath(__file__))
exe_root = os.path.join(make_dir, os.pardir, 'bin')
lib_root = os.path.join(make_dir, os.pardir, 'lib')
rsc_compiler_dir = os.path.join(make_dir, os.pardir, os.pardir, os.pardir, os.pardir, 'Tools', 'RemoteShaderCompiler', 'Compiler', 'PCGL')

def get_standalone_version():
	import re

	standalone_version = ''
	try:
		standalone_version = open(os.path.join(make_dir, 'rsc_version.txt'), 'r').read().strip()
	except:
		raise RuntimeError('Could not retrieve the RSC version string from rsc_version.txt')

	match = re.match('^(V[\d][\d][\d])$', standalone_version) 
	if not match:
		match = re.match('^(V[\d][\d][\d]_[\w]+)$', standalone_version) 
	if not match:
		raise RuntimeError('The RSC version string in rsc_version.txt is not properly formatted')
	return match.group(1)

if __name__ == '__main__':
	standalone_version = get_standalone_version()
	standalone_dir = os.path.join(rsc_compiler_dir, standalone_version)

	for platform in ['win32', 'win64']:
		lib_dir = os.path.join(lib_root, platform)
		exe_dir = os.path.join(exe_root, platform)
		build(configuration, platform, lib_dir, exe_dir)

	if not os.path.exists(standalone_dir):
		os.makedirs(standalone_dir)
	build(portable = True, exe_dir = standalone_dir, platform = 'win64', configuration = 'release')