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

import os
import string
import subprocess
import sys
import multiprocessing

from waflib import Logs, Utils, Options
from waflib.Configure import conf
from cry_utils import WAF_EXECUTABLE
from lumberyard import NON_BUILD_COMMANDS, CURRENT_WAF_EXECUTABLE

# Attempt to import the winregistry module.
try:
    import _winreg
    WINREG_SUPPORTED = True
except ImportError:
    WINREG_SUPPORTED = False
    pass

IB_REGISTRY_PATH = "Software\\Wow6432Node\\Xoreax\\Incredibuild\\Builder"


########################################################################################################
# Validate the Incredibuild Registry settings
def internal_validate_incredibuild_registry_settings(ctx):

    """ Helper function to verify the correct incredibuild settings """
    if Utils.unversioned_sys_platform() != 'win32':
        # Check windows registry only
        return False

    import _winreg

    if not ctx.is_option_true('use_incredibuild'):
        # No need to check IB settings if there is no IB
        return False

    allow_reg_updated = ctx.is_option_true('auto_update_incredibuild_settings') and \
                        not ctx.is_option_true('internal_dont_check_recursive_execution') and \
                        not Options.options.execsolution

    # Open the incredibuild settings registry key to validate if IB is installed properly
    try:
        ib_settings_read_only = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, IB_REGISTRY_PATH, 0, _winreg.KEY_READ)
    except:
        Logs.debug('lumberyard: Cannot open registry entry "HKEY_LOCAL_MACHINE\\{}"'.format(IB_REGISTRY_PATH))
        Logs.warn('[WARNING] Incredibuild does not appear to be correctly installed on your machine.  Disabling Incredibuild.')
        return False

    def _read_ib_reg_setting(reg_key, setting_name, setting_path, expected_value):
        try:
            reg_data, reg_type = _winreg.QueryValueEx(reg_key, setting_name)
            return reg_data == expected_value
        except:
            Logs.debug('lumberyard: Cannot find a registry entry for "HKEY_LOCAL_MACHINE\\{}\\{}"'.format(setting_path,setting_name))
            return False

    def _write_ib_reg_setting(reg_key, setting_name, setting_path, value):
        try:
            _winreg.SetValueEx(reg_key, setting_name, 0, _winreg.REG_SZ, str(value))
            return True
        except WindowsError as e:
            Logs.warn('lumberyard: Unable write to HKEY_LOCAL_MACHINE\\{}\\{} : {}'.format(setting_path,setting_name,e.strerror))
            return False

    valid_ib_reg_key_values = [('MaxConcurrentPDBs', '0')]

    is_ib_ready = True
    for settings_name, expected_value in valid_ib_reg_key_values:
        if is_ib_ready and not _read_ib_reg_setting(ib_settings_read_only, settings_name, IB_REGISTRY_PATH, expected_value):
            is_ib_ready = False

    # If we are IB ready, short-circuit out
    if is_ib_ready:
        return True

    # If we need updates, check if we have 'auto auto-update-incredibuild-settings' set or not
    if not allow_reg_updated:
        Logs.warn('[WARNING] The required settings for incredibuild is not properly configured. ')
        if not ctx.is_option_true('auto_update_incredibuild_settings'):
            Logs.warn("[WARNING]: Set the '--auto-update-incredibuild-settings' to True if you want to attempt to automatically update the settings")
        return False

    # if auto-update-incredibuild-settings is true, then attempt to update the values automatically
    try:
        ib_settings_writing = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, IB_REGISTRY_PATH, 0,  _winreg.KEY_SET_VALUE |  _winreg.KEY_READ)
    except:
        Logs.warn('[WARNING] Cannot access a registry entry "HKEY_LOCAL_MACHINE\\{}" for writing.'.format(IB_REGISTRY_PATH))
        Logs.warn('[WARNING] Please run "{0}" as an administrator or change the value to "0" in the registry to ensure a correct operation of WAF'.format(WAF_EXECUTABLE) )
        return False

    # Once we get the key, attempt to update the values
    is_ib_updated = True
    for settings_name, set_value in valid_ib_reg_key_values:
        if is_ib_updated and not _write_ib_reg_setting(ib_settings_writing, settings_name, IB_REGISTRY_PATH, set_value):
            is_ib_updated = False
    if not is_ib_updated:
        Logs.warn('[WARNING] Unable to update registry settings for incredibuild')
        return False

    Logs.info('[INFO] Registry values updated for incredibuild')
    return True


########################################################################################################
def internal_use_incredibuild(ctx, section_name, option_name, value, verification_fn):
    """ If Incredibuild should be used, check for required packages """

    # GUI
    if not ctx.is_option_true('console_mode'):
        return ctx.gui_get_attribute(section_name, option_name, value)

    if not value or value != 'True':
        return value
    if not Utils.unversioned_sys_platform() == 'win32':
        return value

    _incredibuild_disclaimer(ctx)
    ctx.start_msg('Incredibuild Licence Check')
    (res, warning, error) = verification_fn(ctx, option_name, value)
    if not res:
        if warning:
            Logs.warn(warning)
        if error:
            ctx.end_msg(error, color='YELLOW')
        return 'False'

    ctx.end_msg('ok')
    return value


########################################################################################################
def _incredibuild_disclaimer(ctx):
    """ Helper function to show a disclaimer over incredibuild before asking for settings """
    if getattr(ctx, 'incredibuild_disclaimer_shown', False):
        return
    Logs.info('\nWAF is using Incredibuild for distributed Builds')
    Logs.info('To be able to compile with WAF, various licenses are required:')
    Logs.info('The "IncrediBuild for Make && Build Tools Package"   is always needed')
    Logs.info('If some packages are missing, please ask IT')
    Logs.info('to assign the needed ones to your machine')

    ctx.incredibuild_disclaimer_shown = True


########################################################################################################
def internal_verify_incredibuild_license(licence_name, platform_name):
    """ Helper function to check if user has a incredibuild licence """
    try:
        result = subprocess.check_output(['xgconsole.exe', '/QUERYLICENSE'])
    except:
        error = '[ERROR] Incredibuild not found on system'
        return False, "", error

    if not licence_name in result:
        error = '[ERROR] Incredibuild on "%s" Disabled - Missing IB licence: "%s"' % (platform_name, licence_name)
        return False, "", error

    return True,"", ""


@conf
def invoke_waf_recursively(bld, build_metrics_supported=False, metrics_namespace=None):
    """
    Check the incredibuild parameters and environment to see if we need to invoke waf through incredibuild

    :param bld:                 The BuildContext

    :return:    True to short circuit the current build flow (because an incredibuild command has been invoked), False to continue the flow
    """

    if not WINREG_SUPPORTED:
        return False  # We can't run incredibuild on systems that don't support windows registry

    if not Utils.unversioned_sys_platform() == 'win32':
        return False  # Don't use recursive execution on non-windows hosts

    if bld.is_option_true('internal_dont_check_recursive_execution'):
        return False

    # Skip clean_ commands
    if bld.cmd.startswith('clean_'):
        return False

    # Skip non-build commands
    if bld.cmd in NON_BUILD_COMMANDS:
        return False

    # Don't use IB for special single file operations
    if bld.is_option_true('show_includes'):
        return False
    if bld.is_option_true('show_preprocessed_file'):
        return False
    if bld.is_option_true('show_disassembly'):
        return False
    if bld.options.file_filter != "":
        return False

    # Skip if incredibuild is disabled
    if not bld.is_option_true('use_incredibuild'):
        Logs.warn('[WARNING] Incredibuild disabled by build option')
        return False

    try:
        # Get correct incredibuild installation folder to not depend on PATH
        IB_settings = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, IB_REGISTRY_PATH, 0, _winreg.KEY_READ)
        (ib_folder, type) = _winreg.QueryValueEx(IB_settings, 'Folder')
    except:
        Logs.warn('[WARNING] Incredibuild disabled.  Cannot find incredibuild installation.')
        return False

    # Get the incredibuild profile file
    ib_profile_xml_check = getattr(bld.options, 'incredibuild_profile', '').replace("Code/Tools/waf-1.7.13/profile.xml", "Tools/build/waf-1.7.13/profile.xml")
    if os.path.exists(ib_profile_xml_check):
        ib_profile_xml = ib_profile_xml_check
    else:
        # If the profile doesnt exist, then attempt to use the engine as the base
        ib_profile_xml = bld.engine_node.make_node(ib_profile_xml_check).abspath()
        if not os.path.exists(ib_profile_xml):
            # If the entry doesnt exist, then set to the profile.xml that comes with the engine
            ib_profile_xml = bld.engine_node.make_node("Tools/build/waf-1.7.13/profile.xml").abspath()

    result = subprocess.check_output([str(ib_folder) + '/xgconsole.exe', '/QUERYLICENSE'])

    # Make & Build Tools is required
    if not 'Make && Build Tools' in result:
        Logs.warn('Required Make && Build Tools Package not found.  Build will not be accelerated through Incredibuild')
        return False

    # Determine if the Dev Tool Acceleration package is available.  This package is required for consoles
    dev_tool_accelerated = 'IncrediBuild for Dev Tool Acceleration' in result

    # Android builds need the Dev Tools Acceleration Package in order to use the profile.xml to specify how tasks are distributed
    if 'android' in bld.cmd:
        if not dev_tool_accelerated:
            Logs.warn('Dev Tool Acceleration Package not found! This is required in order to use Incredibuild for Android.  Build will not be accelerated through Incredibuild.')
            return False

    # Windows builds can be run without the Dev Tools Acceleration Package, but won't distribute Qt tasks.
    if not dev_tool_accelerated:
        Logs.warn('Dev Tool Acceleration Package not found.   Qt tasks will not be distributed through Incredibuild')

    # Get all specific commands, but keep msvs to execute after IB has finished
    bExecuteMSVS = False
    if 'msvs' in Options.commands:
        bExecuteMSVS = True

    Options.commands = []
    cmd_line_args = []
    for arg in sys.argv[1:]:
        if arg == 'generate_uber_files':
            continue
        if arg == 'generate_module_def_files':
            continue
        if arg == 'msvs':
            bExecuteMSVS = True
            continue
        if arg == 'configure':
            Logs.warn(
                '[WARNING] Incredibuild disabled, running configure and build in one line is not supported with incredibuild. To build with incredibuild, run the build without the configure command on the same line')
            return False
        if ' ' in arg and '=' in arg:  # replace strings like "--build-options=c:/root with spaces in it/file" with "--build-options=file"
            command, path_val = string.split(arg, '=')
            path_val = os.path.relpath(path_val)
            arg = command + '=' + path_val
        cmd_line_args += [arg]

    if bExecuteMSVS:  # Execute MSVS without IB
        Options.commands += ['msvs']

    command_line_options = ' '.join(cmd_line_args)  # Recreate command line

    # Add special option to not start IB from within IB
    command_line_options += ' --internal-dont-check-recursive-execution=True'

    num_jobs = bld.options.incredibuild_max_cores

    # Build Command Line
    command = CURRENT_WAF_EXECUTABLE + ' --jobs=' + str(num_jobs) + ' ' + command_line_options
    if build_metrics_supported:
        command += ' --enable-build-metrics'
        if metrics_namespace is not None:
            command += ' --metrics-namespace {0}'.format(metrics_namespace)

    sys.stdout.write('[WAF] Starting Incredibuild: ')

    process_call = []
    if dev_tool_accelerated:
        process_call.append(str(ib_folder) + '/xgconsole.exe')

        # If the IB profile is not blank, then attempt to use it
        if len(ib_profile_xml) > 0:
            ib_profile_xml_file = os.path.abspath(ib_profile_xml)
            # Set the profile for incredibuild only if it exists
            if os.path.exists(ib_profile_xml_file):
                process_call.append('/profile={}'.format(ib_profile_xml_file))
            else:
                Logs.warn('[WARN] Incredibuild profile file "{}" does not exist.  Using default incredibuild settings'.format(ib_profile_xml))
    else:
        process_call.append(str(ib_folder) + '/buildconsole.exe')

        # using a profile overrides the handling of max link tasks.  Unfortunately, the make&build tool doesn't support
        # the profile, so we must check the registry settings to ensure that they allow parallel linking up to the
        # count specified in waf.  Incredibuild suggests adding an override parameter to the msbuild command to override
        # this, but since we aren't using this, we warn instead
        try:
            # grab the limitor for number of local jobs that incredibuild will use
            (ib_max_local_cpu,type) = _winreg.QueryValueEx(IB_settings, 'ForceCPUCount_WhenInitiator')
            # grab the limitor that incredibuild will use if a profile is not specified
            (ib_max_link,type) = _winreg.QueryValueEx(IB_settings, 'MaxParallelLinkTargets')
        except:
            Logs.warn('[WARNING] unable to query Incredibuild registry, parallel linking may be sub-optimal')
        else:
            ib_max_local_cpu = int(ib_max_local_cpu)
            if (ib_max_local_cpu == 0):
                ib_max_local_cpu = multiprocessing.cpu_count()
            # executable links are limited to max_parallel_link using a semaphore.  lib/dll links are limited to number
            # of cores since they are generally single threaded
            min_setting_needed = int(min(ib_max_local_cpu, bld.options.max_parallel_link))
            ib_max_link = int(ib_max_link)
            if ib_max_link < min_setting_needed:
                Logs.warn('[WARNING] Incredibuild configuration \'MaxParallelLinkTargets\' limits link tasks to %d, increasing to %d will improve link throughput' % (ib_max_link, min_setting_needed))

    process_call.append('/command=' + command)
    process_call.append('/useidemonitor')
    process_call.append('/nologo')

    Logs.debug('incredibuild: Cmdline: ' + str(process_call))
    if subprocess.call(process_call, env=os.environ.copy()):
        bld.fatal("[ERROR] Build Failed")

    return True
