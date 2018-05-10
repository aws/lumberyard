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

from waflib import Context, Utils, Logs
from waflib.Configure import conf
from third_party import ThirdPartySettings
import sys
import os

LMBR_SETUP_REL_PATH = os.path.join('Tools', 'LmbrSetup')

LY_LAUNCHER_CODE_REL_PATH = os.path.join('Code', 'Tools', 'AZInstaller')

DEFAULT_LY_LAUNCHER_PARAMS = ['--none',
                              '--enablecapability compilegame',
                              '--enablecapability compileengine',
                              '--enablecapability compilesandbox',
                              '--no-modify-environment']


def exec_command(cmd, realtimeOutput=False, **kw):
    subprocess = Utils.subprocess

    if isinstance(cmd, list):
        cmd = ' '.join(cmd)

    try:
        # warning: may deadlock with a lot of output (subprocess limitation)
        kw['stdout'] = kw['stderr'] = subprocess.PIPE
        p = subprocess.Popen(cmd, **kw)

        if realtimeOutput == True:
            out = ""

            for line in iter(p.stdout.readline, b''):
                print line,
                out += line

            p.stdout.close()

            err = p.stderr.read()
            p.stderr.close()

            p.wait()

        else:
            (out, err) = p.communicate()


        if out:
            out = out.decode(sys.stdout.encoding or 'iso8859-1')
        if err:
            err = err.decode(sys.stdout.encoding or 'iso8859-1')

        return p.returncode, out, err

    except OSError as e:
        return [-1, "OS Error", '%s - %s' % (str(e.errno), e.strerror)]
    except:
        return [-1, "Exception",  sys.exc_info()[0]]


def get_ly_launcher_executable(ctx):
    """
    Build the handle that represents where ly launcher (setup assistant batch) is expected to be

    :param ctx: The configuration context
    :return:    The expected path of the setup assistant executable
    """

    ly_launcher_name = 'SetupAssistantBatch'
    ly_launcher_path = None

    sys_platform = Utils.unversioned_sys_platform()
    if sys_platform == 'linux':
        ly_launcher_path = ctx.Path(os.path.join(LMBR_SETUP_REL_PATH, 'Linux', ly_launcher_name))
    elif sys_platform == 'darwin':
        ly_launcher_path = ctx.Path(os.path.join(LMBR_SETUP_REL_PATH, 'Mac', ly_launcher_name))
    elif sys_platform == 'win32':
        win_launcher_name = '{}.exe'.format(ly_launcher_name)
        ly_launcher_path = ctx.Path(os.path.join(LMBR_SETUP_REL_PATH, 'Win', win_launcher_name))
    else:
        ctx.fatal("[ERROR] Host '{}' not supported".format(sys_platform))

    return ly_launcher_path


@conf
def run_bootstrap_tool(ctx, ly_params, setup_assistant_third_party_override):

    verbose_mode = Logs.verbose > 0
    bootstrap_script_dir = 'Code/Tools/AZInstaller/scripts'
    bootstrap_script = ctx.srcnode.find_resource(os.path.join(bootstrap_script_dir, 'setup_symlinks.py'))

    # set up the required parameters to pass into Setup Assistant
    if ly_params is None:
        ly_params = ' '.join(DEFAULT_LY_LAUNCHER_PARAMS)

    # setup third party override
    set_3p_folder_path = ""
    if setup_assistant_third_party_override is not None and len(setup_assistant_third_party_override.strip())>0:
        if os.path.exists(setup_assistant_third_party_override):
            set_3p_folder_path = setup_assistant_third_party_override
            set_3p_folder = ' --3rdpartypath \"{}\"'.format(set_3p_folder_path)
            ly_params += set_3p_folder
        else:
            Logs.warn('[WARN] 3rd Party Bootstrap option (--3rdpartypath=\"{}\") is not a valid path.  Ignoring.'.format(setup_assistant_third_party_override))

    start_message = ''
    ly_cmd = []
    fail_message = ''

    # Run SetupAssistant bootstrap script to set up your workspace if available
    if bootstrap_script is not None:
        Logs.info('Configuring your setup...')

        sys.path.append(os.path.join(ctx.path.abspath(), bootstrap_script_dir))
        import argparse
        import setup_symlinks

        setup_symlinks_arg = argparse.Namespace()
        setup_symlinks_arg.engineRootPath = ctx.path.abspath()
        setup_symlinks_arg.thirdPartyPath = set_3p_folder_path
        setup_symlinks_arg.verbose = verbose_mode
        setup_symlinks_arg.disableDefaultDevCapabilities = False
        setup_symlinks_arg.saveCurrentCapabilities = True
        setup_symlinks_arg.mode = "create"
        setup_symlinks_arg.singleThread = False

        if setup_symlinks.setup_symlinks(setup_symlinks_arg) is False:
            ctx.fatal('[ERROR] Failed to setup your symlinks.')

    # Cannot find setup script, run executable instead
    else:
        # Get the expected ly_launcher executable
        ly_launcher_path = get_ly_launcher_executable(ctx)
        if not os.path.exists(ly_launcher_path):
            ctx.fatal('[ERROR] Unable to launch setup assistant. Cannot find executable in \'%s\'.' % ly_launcher_path)

        Logs.info('Running SetupAssistant ...')
        exit_code, out, err = exec_command(['"' + ly_launcher_path + '"', ly_params], realtimeOutput=verbose_mode, shell=True)
    
        if exit_code != 0:
            ctx.fatal('\n'.join(['{} Output: '.format('Lumberyard Setup Assistant failed to run!'), str(out), str(err)]))

        if verbose_mode == False:
            Logs.info(out.strip())



@conf
def run_linkless_bootstrap(conf):

    if not hasattr(conf,"tp"):
        conf.tp = ThirdPartySettings(conf)
        conf.tp.initialize()

