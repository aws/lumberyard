#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
# Certain MSVC9.0 runtime environments may be incompatible with the one we intend to use
# to invoke python, here we search for any paths that contain msvcr90.dll and remove them
# to ensure they cannot be brought into our environment - failure to do this may result
# in runtime library mismatches and/or CRT violations.
# This has to be done before anything else

import os
import sys
import json
import tempfile
import imp
from collections import defaultdict

try:
    # Prevent non-sxs version of msvcr90.dll from being loaded by path environment variable.
    if 'path' in os.environ:
        system_paths_to_check = os.environ['path'].split(os.pathsep)
        patched_system_paths = []
        for system_path_to_check in system_paths_to_check:
            if os.path.isdir(system_path_to_check):
                if "msvcr90.dll" not in map(str.lower, os.listdir(system_path_to_check)):
                    patched_system_paths.append(system_path_to_check)
        os.environ['path'] = os.pathsep.join(patched_system_paths)
except Exception, e:
    print 'error - {}'.format(e)
    raise


def run_cmd(cmd, **kw):
    import subprocess
    print('cmd: %r' % cmd)
    kw['shell'] = isinstance(cmd, str)
    kw['stdout'] = kw['stderr'] = subprocess.PIPE
    try:
        p = subprocess.Popen(cmd, **kw)
        (out, err) = p.communicate()
    except Exception as e:
        raise RuntimeError('Execution failure: %s' % str(e), ex=e)

    if not isinstance(out, str):
        out = out.decode(sys.stdout.encoding or 'iso8859-1')
    if not isinstance(err, str):
        err = err.decode(sys.stdout.encoding or 'iso8859-1')

    if p.returncode:
        raise RuntimeError('Command %r returned %r' % (cmd, p.returncode))

    return (out, err)


def run_azcg(argsfile):
    azcg_exe = os.path.join(os.path.dirname(__file__), 'AzCodeGenerator.exe')
    if not os.path.exists(azcg_exe):
        azcg_exe = os.path.join(os.path.abspath(os.path.curdir), 'AzCodeGenerator.exe')
    json_file_path = os.path.join(tempfile.gettempdir(), '{}_{}.json'.format(tempfile.gettempprefix(), os.times()[0]))
    azcg_args = ' '.join([
        '-intermediate-file=' + json_file_path,
        "-noscripts"
    ])
    cmd = '{} {} {}'.format(azcg_exe, azcg_args, argsfile)
    (output, errors) = run_cmd(cmd)
    if errors:
        raise RuntimeError('AzCodeGenerator produced errors: ' + errors)

    json_file = open(json_file_path, 'r')
    json_object = json.load(json_file)
    json_file.close()
    os.remove(json_file_path)
    return json_object


def run_scripts(args, parser_json):
    # configure sys.path like the codegen does
    sys.path = [os.path.dirname(__file__)] # executable should be next to this script
    sys.path += args['python_path'] # any paths passed into the codegen tool

    # fake azcg_extension by injecting stubs (normally done by codegen tool)
    # see PEP-302: https://www.python.org/dev/peps/pep-0302/
    class AZCGFinder(object):
        def find_module(self, fullname, path=None):
            class AZCGLoader(object):
                def load_module(self, fullname):
                    if fullname == 'azcg_extension':
                        if fullname in sys.modules:
                            return sys.modules['fullname']

                        azcg_extension = sys.modules.setdefault('azcg_extension', imp.new_module('azcg_extension'))
                        azcg_extension.__file__ = "<%s>" % self.__class__.__name__
                        azcg_extension.__loader__ = self
                        azcg_extension.__package__ = 'azcg_extension'
                        def RegisterOutputFile(file_path, should_add_to_build):
                            pass
                        def RegisterDependencyFile(file_path):
                            pass
                        def OutputError(error):
                            print (error)
                        def OutputPrint(msg):
                            pass
                        azcg_extension.RegisterOutputFile = RegisterOutputFile
                        azcg_extension.RegisterDependencyFile = RegisterDependencyFile
                        azcg_extension.OutputError = OutputError
                        azcg_extension.OutputPrint = OutputPrint
                        return azcg_extension
                    return None
            if fullname == 'azcg_extension':
                return AZCGLoader()
            return None

    # when scripts import azcg_extension, this finder will handle it
    sys.meta_path.append(AZCGFinder())

    # import the launcher, run_scripts is the entry point
    launcher = __import__('launcher')
    launcher.run_scripts(','.join(args['codegen_script']), json.dumps(parser_json), args['input_path'][0], args['output_path'][0], args['input_file'][0])


def parse_argsfile(argspath):
    args = defaultdict(list)
    argsfile = open(argspath, 'r')
    for line in argsfile:
        pieces = line.split(' ')
        if len(pieces) is 1:
            pieces = line.split('=')
        if len(pieces) == 2:
            key, val = pieces[0], pieces[1]
            key = key[1:].replace('-', '_')
            val = val.strip('"\n')
            args[key] += [val]

    return args


if __name__ == '__main__':
    argsfile = None
    scripts = []
    for arg in sys.argv:
        if arg.startswith('@'):
            argsfile = arg

    args = parse_argsfile(argsfile[1:])

    if not argsfile:
        raise RuntimeError('No args file specified for code generator')

    parser_json = run_azcg(argsfile)
    run_scripts(args, parser_json)
