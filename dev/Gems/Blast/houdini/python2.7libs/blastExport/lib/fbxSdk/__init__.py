import sys
import os

print('Fbx imported from application %s using the %s interpreter' % (sys.executable, sys.version))

__bitDepth = 'win64'
__pythonVersion = sys.winver.replace('.', '')
__thisDirectory = os.path.dirname(__file__).replace('\\', '/')
__pydPath = '%s/sdk_2018_%s' % (__thisDirectory, __pythonVersion)
sys.path.append(__pydPath)
