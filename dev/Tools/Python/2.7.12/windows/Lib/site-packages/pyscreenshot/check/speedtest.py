from entrypoint2 import entrypoint
import pyscreenshot
import tempfile
import time
import os
import shutil


def run(force_backend, n, to_file, bbox=None):
    tmpdir = tempfile.mkdtemp(prefix='pyscreenshot_speedtest_')
    start = time.time()
    for _ in range(n):
        if to_file:
            filename=os.path.join(tmpdir, 'speedtest.png')
            pyscreenshot.grab_to_file(filename, backend=force_backend, childprocess=True)
        else:
            pyscreenshot.grab(bbox=bbox, backend=force_backend, childprocess=True)
    end = time.time()
    dt = end - start

    s = ''
    s += '%-20s' % force_backend
    s += '\t'
    s += '%-4.2g sec' % dt
    s += '\t'
    s += '(%5d ms per call)' % (1000.0 * dt / n)
    print(s)
    shutil.rmtree(tmpdir)
    


def run_all(n, to_file, bbox=None):
    print('')

    s = ''
    s += 'n=%s' % n
    s += '\t'
    s += ' to_file: %s' % to_file
    s += '\t'
    s += ' bounding box: %s' % (bbox,)
    print(s)

    print('------------------------------------------------------')

    for x in pyscreenshot.backends():
        try:
            run(x, n, to_file, bbox)
        except pyscreenshot.FailedBackendError as e:
            print(e)


def speedtest():
    n = 10
    run_all(n, True)
    run_all(n, False)
    run_all(n, False, (10, 10, 20, 20))
    
@entrypoint
def main(virtual_display=False):
    if virtual_display:
        from pyvirtualdisplay import Display
        with Display(visible=0):
            speedtest()
    else:
        speedtest()
        
