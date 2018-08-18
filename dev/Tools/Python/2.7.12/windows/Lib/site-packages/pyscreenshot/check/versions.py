from entrypoint2 import entrypoint

from pyscreenshot import backend_version
import pyscreenshot


def print_name_version(name, version):
    s = '%-20s %s' % (name, version)
    print(s)


@entrypoint
def print_versions():
    print_name_version('pyscreenshot', pyscreenshot.__version__)
    
    for name in pyscreenshot.backends():
        v=backend_version(name, childprocess=True)
        if not v:
            v = 'missing'
        print_name_version(name, v)
