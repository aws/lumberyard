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
# Original file Copyright Crytek GMBH or its affiliates, used under license.
#
import os
import json

from waflib.Build import BuildContext
from waflib import Context, TaskGen, Logs
from waflib.Task import Task
from waflib.Configure import conf

#############################################################################
class use_file_generator(BuildContext):
    ''' Util class to generate a use dependency file for a single module '''
    cmd = 'generate_module_def_files'
    fun = 'build'

#############################################################################
# Sets to store 'exempt' modules for use maps.  These modules are identified as ones that do not implicitly belong
# within inferred use maps
EXEMPT_USE_MODULES = ['AZCodeGenerator']
EXEMPT_SPEC_MODULES = ['MetricsPlugin','ProjectConfigurator',
                      ]

@conf
def mark_module_exempt(ctx, target):
    EXEMPT_USE_MODULES.append(target)


def get_module_def_folder(ctx):
    return ctx.get_bintemp_folder_node().make_node('module_def')


def _is_module_exempt(ctx, target_name):

    # Ignore the Exempt modules that are not part of the spec system
    if target_name in EXEMPT_USE_MODULES:
        return True

    # Ignore any launcher derived uses
    if target_name.endswith('Launcher'):
        return True

    return False

# Cache the module use map to reduce file reads
cached_module_use_map = {}
cached_module_def_map = {}


MODULE_DEF_FILENAME_FORMAT = '{}_module_def.json'


def read_module_def_file(ctx, target_name):
    """
    Read a module's definition file
    :param ctx:          Context
    :param target_name:  Name of the module
    :return:             Modules defined in the use_map if possible
    """
    filename = MODULE_DEF_FILENAME_FORMAT.format(target_name)
    use_map_file_folder = get_module_def_folder(ctx)
    use_map_file = use_map_file_folder.make_node(filename)
    if os.path.exists(use_map_file.abspath()):
        # Return the cached module def if possible
        if target_name in cached_module_def_map:
            return cached_module_def_map[target_name]
        # Read, cache, and return the module def
        module_def = ctx.parse_json_file(use_map_file)
        if module_def['target'] == target_name:
            cached_module_def_map[target_name] = module_def
            return module_def

    return None


@conf
def get_module_platforms(ctx, target, preprocess=True):

    platforms = []
    module_def = read_module_def_file(ctx, target)
    if module_def is not None:
        platforms += module_def['platforms']

    if preprocess:
        return ctx.preprocess_platform_list(platforms)
    else:
        return platforms


@conf
def get_module_configurations(ctx, target, platform=None, preprocess=True):

    configurations = []
    module_def = read_module_def_file(ctx, target)
    if module_def is not None:
        configurations += module_def['configurations']

    if preprocess:
        return ctx.preprocess_configuration_list(target, platform, configurations)
    else:
        return configurations


def _read_module_def_file(ctx, target_name, parent_module, parent_spec):
    """
    Read a module's use_map file if possible
    :param ctx:          Context
    :param target_name:  Name of the module
    :return:             Modules defined in the use_map if possible
    """
    if _is_module_exempt(ctx, target_name):
        return None

    # If a module 'uses' a uselib module, that means that the module intends to allow the
    # dependents of the module to inherit the uselib as well, which is valid.  Uselibs do
    # not have any dependent uses.
    third_party_uselibs = getattr(ctx.all_envs[''], 'THIRD_PARTY_USELIBS', [])
    if target_name in third_party_uselibs:
        return None

    module_def = read_module_def_file(ctx, target_name)
    if module_def is None:
        if parent_module is not None:
            pass
        elif parent_spec is not None:
            # Only show the warning for modules that are on exempt module spec list and they are not a GEM
            if not target_name in EXEMPT_SPEC_MODULES and not ctx.is_gem(target_name):
                Logs.warn('[WARN] Invalid module {} defined as a use dependency in waf spec \'{}\''.format(target_name,
                                                                                                           parent_spec))
        return None
    return module_def

@conf
def get_export_internal_3rd_party_lib(ctx, target, parent_spec):
    """
    Determine if a module has the 'export_internal_3rd_party_lib' flag set to True

    :param ctx:     Context
    :param target:  Name of the module
    :param parent_spec: Name of the parent spec
    :return:    True if 'export_internal_3rd_party_lib' ios set to True, default to False if not specified
    """
    module_def = cached_module_def_map.get(target, _read_module_def_file(ctx, target, None, parent_spec))
    if not module_def:
        return False
    return module_def.get('export_internal_3rd_party_lib', False)


@conf
def get_module_uses(ctx, target, parent_spec):
    """
    Get all the module uses for a particular module (from the generated use_map)
    :param ctx:     Context
    :param target:  Name of the module
    :param parent_spec: Name of the parent spec
    :return:        Set of all modules that this module is dependent on
    """

    def _get_module_use(ctx, target_name, parent_module, parent_spec):
        """
        Determine all of the uses for a module recursivel
        :param ctx:          Context
        :param target_name:  Name of the module
        :return:        Set of all modules that this module is dependent on
        """
        if target_name not in cached_module_use_map:
            uses = []
            cached_module_use_map[target_name] = uses
            module_def = _read_module_def_file(ctx, target_name, parent_module, parent_spec)
            if module_def is not None:
                uses += module_def['uses']
            sub_uses = []
            for use in uses:
                if target_name != use:
                    sub_uses += _get_module_use(ctx, use, target_name, None)
            uses += sub_uses
        return cached_module_use_map[target_name]

    return _get_module_use(ctx, target, None, parent_spec)

# Cache the spec module use map to reduce file reads
cached_spec_module_use_map = {}


def _generate_cache_key(ctx, spec_name, platform, configuration):
    """
    Generate a composite key for (optional) spec_name, platform, and configuration
    :param ctx:             The context
    :param spec_name:       name of the spec (or None, use the current spec)
    :param platform:        the platform (or None, use the current platform)
    :param configuration:   the configuration (or None, use the current configuration)
    :return:                The composite key
    """
    if hasattr(ctx, 'env'):
        if not platform:
            platform = ctx.env['PLATFORM']
        if not configuration:
            configuration = ctx.env['CONFIGURATION']
    else:
        if not platform:
            platform = 'none'
        if not configuration:
            configuration = 'none'
    composite_key = spec_name + '_' + platform + '_' + configuration
    return composite_key


@conf
def get_all_module_uses(ctx, spec_name=None, platform=None, configuration=None):
    """
    Get all of the module uses for a spec
    :param ctx:             Context
    :param spec_name:       name of the spec (or None, use the current spec)
    :param platform:        the platform (or None, use the current platform)
    :param configuration:   the configuration (or None, use the current configuration)
    :return:
    """

    if not spec_name: # Fall back to command line specified spec if none was given
        spec_name = ctx.options.project_spec

    composite_key = _generate_cache_key(ctx, spec_name, platform, configuration)
    if composite_key in cached_spec_module_use_map:
        return cached_spec_module_use_map[composite_key]

    all_modules = set()
    spec_modules = ctx.spec_modules(spec_name,platform,configuration)

    for spec_module in spec_modules:
        if not _is_module_exempt(ctx,spec_module):
            all_modules.add(spec_module)
        all_spec_module_uses = get_module_uses(ctx,spec_module,spec_name)
        [all_modules.add(i) for i in all_spec_module_uses if not _is_module_exempt(ctx, i)]

    cached_spec_module_use_map[composite_key] = all_modules
    return all_modules


@TaskGen.taskgen_method
@TaskGen.feature('generate_module_def_files')
def gen_create_use_map_file(tgen):
    """
    Generate the use_map file taskgen for a single target
    """

    # Make sure the use_map folder exists
    use_map_file_folder = get_module_def_folder(tgen.bld)
    use_map_file_folder.mkdir()

    # Determine the source wscript and output usemap file name for the current target
    wscript_src = tgen.path.make_node('wscript')
    module_def_file = use_map_file_folder.make_node(MODULE_DEF_FILENAME_FORMAT.format(tgen.target))

    # Get the use module list from the processing of the target's wscript file
    use_module_list = getattr(tgen, 'use_module_list')
    platform_list = getattr(tgen, 'platform_list')
    config_list = getattr(tgen, 'configuration_list')
    export_internal_3p_libs = getattr(tgen, 'export_internal_3p_libs')

    # Create the taskgen for the use_map generation
    tsk = tgen.create_task('gen_module_def_file', wscript_src, module_def_file )
    setattr(tsk, 'EXPORT_INTERNAL_3P_LIBS', export_internal_3p_libs)
    setattr(tsk, 'USE_MAP_LIST', use_module_list)
    setattr(tsk, 'PLATFORM_LIST', [] if platform_list is None else platform_list)
    setattr(tsk, 'CONFIG_LIST', [] if config_list is None else config_list)
    setattr(tsk, 'TARGET', tgen.target)


class gen_module_def_file(Task):
    """
    Definition for the use_map file generator task
    """
    color = 'BLUE'

    def generate_use_map_json(self):

        def _to_json_list(source_list):
            json_list = []
            if source_list is not None:
                for source in source_list:
                    json_list.append('\"' + source + '\"')
                return ','.join(json_list)
            else:
                return ''

        module_def = {"target": self.TARGET,
                      "platforms": self.PLATFORM_LIST,
                      "configurations": self.CONFIG_LIST,
                      "uses": self.USE_MAP_LIST}

        if self.EXPORT_INTERNAL_3P_LIBS:
            module_def["export_internal_3rd_party_lib"] = True

        module_def_json = json.dumps(module_def)
        return module_def_json

    def run(self):
        use_map_content = self.generate_use_map_json()
        self.outputs[0].write(use_map_content)
        return 0


@conf
def get_all_modules(self):
    return cached_module_def_map.keys()


@conf
def is_module_exempt(ctx, target):
    return _is_module_exempt(ctx, target)

