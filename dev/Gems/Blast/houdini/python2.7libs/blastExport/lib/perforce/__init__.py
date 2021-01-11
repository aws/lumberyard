import sys
import os

print('Perforce imported from application %s using the %s interpreter' % (sys.executable, sys.version))

__bitDepth = 'win64'
__pythonVersion = sys.winver.replace('.', '')
__thisDirectory = os.path.dirname(__file__).replace('\\', '/')
__pydPath = '%s/p4_%s' % (__thisDirectory, __pythonVersion)

sys.path.append(__pydPath)

import P4