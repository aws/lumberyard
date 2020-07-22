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
from waflib import Errors, Options, Utils
from waflib.Configure import conf, Logs, ConfigurationContext

PLATFORM = 'win_x64'


# Required load_<PLATFORM>_common_settings(ctx)
@conf
def load_win_x64_common_settings(ctx, vs_version):
    env = ctx.env

    vs_platform = '{}_vs{}'.format(PLATFORM, vs_version)

    # Attempt to detect the C++ compiler for Visual Studio ( VS Version >=15.0 )
    winkit_option_name = 'win_vs{}_winkit'.format(vs_version)
    if not hasattr(ctx.options, winkit_option_name):
        raise Errors.WafError('[Error] VS Version {} is missing winkit option {}'.format(vs_version, winkit_option_name))
    windows_kit = getattr(ctx.options, winkit_option_name)

    vcvarsall_args_option_name = 'win_vs{}_vcvarsall_args'.format(vs_version)
    if not hasattr(ctx.options, vcvarsall_args_option_name):
        raise Errors.WafError('[Error] VS Version {} is missing vcvarsall args option {}'.format(vs_version, vcvarsall_args_option_name))
    vcvarsall_args = getattr(ctx.options, vcvarsall_args_option_name)

    detect_visual_studio_func_name = 'detect_visual_studio_{}'.format(vs_version)
    if not hasattr(ctx, detect_visual_studio_func_name):
        raise Errors.WafError('[Error] VS Version {} is missing Visual Studio detection function args {}'.format(
        vs_version, detect_visual_studio_func_name))
    detect_visual_studio_func = getattr(ctx, detect_visual_studio_func_name)
    detect_visual_studio_func(windows_kit, vcvarsall_args, fallback_to_newer_vs_version=True)

    # Detect the QT binaries, if the current capabilities selected requires it.
    _, enabled, _, _ = ctx.tp.get_third_party_path(vs_platform, 'qt')
    if enabled:
        ctx.find_qt5_binaries(vs_platform)

    compiler_flags = []
    if ctx.options.use_uber_files:
        compiler_flags.append('/bigobj')

    env['CFLAGS'] += compiler_flags
    env['CXXFLAGS'] += compiler_flags

    if not env['CODE_GENERATOR_PATH']:
        raise Errors.WafError('[Error] AZ Code Generator path not set for target platform {}'.format(PLATFORM))

    ctx.find_dx12(windows_kit)

    ctx.setup_copy_hosttools()

    ctx.load_cryengine_common_settings()

    ctx.load_msvc_common_settings()

    ctx.load_windows_common_settings()

    ctx.register_win_x64_external_optional_cuda(vs_platform)


# Required load_<PLATFORM>_configuration_settings(ctx, platform_configuration)
@conf
def load_win_x64_configuration_settings(ctx, platform_configuration, vs_version):
    vsstring = 'vs{}'.format(vs_version)
    if platform_configuration.settings.does_configuration_match('debug'):
        ctx.register_win_x64_external_ly_identity(vsstring, 'Debug')
        ctx.register_win_x64_external_ly_metrics(vsstring, 'Debug')

    elif platform_configuration.settings.does_configuration_match('profile'):
        ctx.register_win_x64_external_ly_identity(vsstring, 'Release')
        ctx.register_win_x64_external_ly_metrics(vsstring, 'Release')


# Optional is_<PLATFORM>_available(ctx)
@conf
def is_win_x64_available(ctx, vs_version):
    return True


@conf
def inject_msvs_command(ctx, vs_version):
    """
    Make sure the msvs command is injected into the configuration context
    """
    vsstring = 'vs{}'.format(vs_version)
    if not isinstance(ctx, ConfigurationContext):
        return
    if not ctx.is_option_true('generate_{}_projects_automatically'.format(vsstring)):
        return

    if ctx.is_target_platform_enabled('win_x64_{}'.format(vsstring)):
        # Inject the Visual Studio creation command if win_x64_<vs_version> is enabled
        Options.commands.append('msvs_{}'.format(vs_version))
