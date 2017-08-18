from build import build
import os

configuration = 'release'

make_dir = os.path.dirname(os.path.realpath(__file__))
exe_root = os.path.join(make_dir, os.pardir, 'bin')
lib_root = os.path.join(make_dir, os.pardir, 'lib')

if __name__ == '__main__':
	for platform in ['linux', 'android-armeabi-v7a']:
		lib_dir = os.path.join(lib_root, platform)
		build(configuration, platform, lib_dir)