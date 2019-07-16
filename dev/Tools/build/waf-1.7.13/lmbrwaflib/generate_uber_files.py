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

from waflib.Build import BuildContext
from waflib import TaskGen, Options
from waflib.TaskGen import extension
from waflib.Task import Task,RUN_ME
from waflib.Configure import conf, conf_event, ConfigurationContext
from collections import OrderedDict, defaultdict

UBER_HEADER_COMMENT = '// UBER SOURCE'
#############################################################################
class uber_file_generator(BuildContext):
    ''' Util class to generate Uber Files after configure '''
    cmd = 'generate_uber_files'
    fun = 'build'

@TaskGen.taskgen_method
@extension('.xpm')
@extension('.def')
@extension('.rtf')
@extension('.action_filters')
@extension('.man')
@extension('.icns')
@extension('.xml')
@extension('.rc2')
@extension('.lib')
@extension('.png')
@extension('.manifest')
@extension('.cfi')
@extension('.cfx')
@extension('.ext')
@extension('.in')
@extension('.am')
@extension('.h')
@extension('.H')
@extension('.hxx')
@extension('.hpp')
@extension('.hxx')
@extension('.inl')
@extension('.json')
@extension('.txt')
@extension('.bmp')
@extension('.ico')
@extension('.S')
@extension('.pssl')
@extension('.cur')
@extension('.actions')
@extension('.lua')
@extension('.cfg')
@extension('.csv')
@extension('.appxmanifest')
@extension('.hlsl')
@extension('.qml')
@extension('.jpg')
@extension('.ttf')
@extension('.ini')
@extension('.tpl')
@extension('.py')
@extension('.natvis')
@extension('.natstepfilter')
@extension('.natjmc')
def header_dummy(tgen,node):
    pass

# Finds all files that are expected to be auto-ubered, sorts them and builds a manifest of uber
# files based on size, then returns a re-mapped file list for use by the rest of the spec system
@conf
def map_uber_files(ctx, original_file_list, token, target_name=None):

    uber_file_folder = ctx.bldnode.make_node('uber_files').make_node(ctx.path.name)
    uber_file_folder.mkdir()

    uber_size_limit = int(ctx.options.uber_file_size)

    auto_source_nodes = []
    file_to_filter = {}

    if not target_name:
        target_name = ctx.path.name

    # copy the input file list and remove the auto group, to be replaced with the
    # generated uber files and their contents
    remapped_file_list = dict(original_file_list)
    if remapped_file_list.has_key('auto'):
        remapped_file_list.pop('auto')

    if 'auto' in original_file_list:
        for (filter_name, file_list) in original_file_list['auto'].items():
            for file in file_list:
                if ctx.is_cxx_file(file):
                    if os.path.isabs(file):
                        file_node = ctx.root.make_node(file)
                    else:
                        file_node = ctx.path.make_node(file)
                    auto_source_nodes.append(file_node)
                    file_to_filter[file_node] = filter_name
                else: # ensure that non-source files make it to the output file list, put them in the none group
                    if not 'none' in remapped_file_list:
                        remapped_file_list['none'] = {}
                    if not filter_name in remapped_file_list['none']:
                        remapped_file_list['none'][filter_name] = []
                    remapped_file_list['none'][filter_name].append(file)

    # sort auto files by name and combine into uber files by size limit, this ensures
    # deterministic output and generally files with similar names often reference
    # similar headers and code
    auto_source_nodes = sorted(auto_source_nodes, key=lambda node: node.abspath())

    auto_uber_files = defaultdict(list)

    def next_uber_file():
         return uber_file_folder.make_node('%s_auto%s_%d.cpp' % (target_name, token, len(auto_uber_files)))

    # Create the initial uber auto file
    total_size = 0
    uber_file = next_uber_file()

    for file_node in auto_source_nodes:
        file_size = os.path.getsize(file_node.abspath())

        # if we've compiled at least one file into this uber file and we'd be over the size limit
        if total_size + file_size > uber_size_limit and uber_file in auto_uber_files and (auto_uber_files[uber_file]) > 0:
            uber_file = next_uber_file()
            total_size = 0

        auto_uber_files[uber_file].append(file_node)
        total_size += file_size

    # insert the newly generated uber files into the file list
    for (uber_file_node, combined_file_nodes) in auto_uber_files.items():
        remapped_file_list[uber_file_node.name] = {}
        for file_node in combined_file_nodes:
            filter = file_to_filter[file_node]
            if not filter in remapped_file_list[uber_file_node.name]:
                remapped_file_list[uber_file_node.name][filter] = []
            remapped_file_list[uber_file_node.name][filter].append(file_node.path_from(ctx.path))

    return remapped_file_list

@TaskGen.taskgen_method
@TaskGen.feature('generate_uber_file')
def gen_create_uber_file_task(tgen):

    uber_file_folder = tgen.bld.bldnode.make_node('uber_files').make_node(tgen.target)
    uber_file_folder.mkdir()

    # Iterate over all uber files, to collect files per Uber file
    uber_file_list = getattr(tgen, 'uber_file_list')
    for (uber_file,project_filter) in uber_file_list.items():
        if uber_file == 'none' or uber_file == 'NoUberFile': # TODO: Deprecate 'NoUberfile'
            continue # Don't create a uber file for the special 'none' name
        files = []
        for (k_1,file_list) in project_filter.items():
            for file in file_list:
                if tgen.bld.is_cxx_file(str(file)):
                    files.append(file)

        file_nodes = tgen.to_nodes(files)
        # Create Task to write Uber File
        tsk = tgen.create_task('gen_uber_file', file_nodes, uber_file_folder.make_node(uber_file) ) 
        setattr( tsk, 'UBER_FILE_FOLDER', uber_file_folder )
        tsk.pch = getattr(tgen, 'pch')
                    
class gen_uber_file(Task):
    color =  'BLUE'

    def __str__(self):
        """
        string to display to the user
        """
        tgt_str = ' '.join([a.nice_path() for a in self.outputs])
        message = "Generating Uber Files ({} source files) -> {}\n".format(len(self.inputs),tgt_str)
        return message

    def compute_uber_file(self):

        lst = ['{}\n'.format(UBER_HEADER_COMMENT)]
        uber_file_folder = self.UBER_FILE_FOLDER

        if not self.pch == '':
            lst += ['#include "%s"\n' % self.pch.replace('.cpp','.h')]
        module_path_base = os.path.normcase(self.generator.path.abspath())
        for node in self.inputs:
            node_path = os.path.normcase(node.abspath())
            if node_path.startswith(module_path_base):
                module_relative_path = node.abspath()[len(module_path_base):].lstrip("\\/")
                lst += ['#include <{}>\n'.format(module_relative_path)]
            else:
                lst += ['#include <{}>\n'.format(node.abspath())]

        uber_file_content = ''.join(lst)        
        return uber_file_content
        
    def run(self):
        uber_file_content = self.compute_uber_file()
        self.outputs[0].write(uber_file_content)
        return 0
        
    def runnable_status(self):  
        if super(gen_uber_file, self).runnable_status() == -1:
            return -1

        uber_file = self.outputs[0]
        # If no uber file exist, we need to write one
        try:
            stat_tgt = os.stat(uber_file.abspath())
        except OSError: 
            return RUN_ME
            
        # We have a uber file, lets compare content
        uber_file_content = self.compute_uber_file()
            
        current_content = self.outputs[0].read()
        if uber_file_content == current_content:
            return -2 # SKIP_ME
            
        return RUN_ME   # Always execute Uber file Generation


@conf_event(after_methods=['update_valid_configurations_file'])
def inject_generate_uber_command(conf):
    """
    Make sure generate_uber_files command is injected into the configure pipeline
    """
    if not isinstance(conf, ConfigurationContext):
        return

    # Insert the generate_uber_files command, it has the highest priority
    Options.commands.append('generate_uber_files')

