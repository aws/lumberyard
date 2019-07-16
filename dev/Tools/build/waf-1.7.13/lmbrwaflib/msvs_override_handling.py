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

## Description: Helper to configure build customizations requested by visual studio
from waflib.TaskGen import before_method, feature
from waflib import Utils

import os
import sys
from waflib.Configure import conf
from waflib import Logs
from cry_utils import compare_config_sets

###############################################################################
def _set_override(override_options_map, node_name, node_value, env, proj_path):
    
    def _set_override_value(override_options_map, key, value):
        try:
            override_options_map[key] += value
        except:
            override_options_map[key] = value
        
    # [WAF_ExcludeFromUberFile] 
    if node_name == 'WAF_ExcludeFromUberFile':
        if node_value.lower() == 'true':        
                override_options_map['exclude_from_uber_file'] = True
    # [WAF_DisableCompilerOptimization]
    elif node_name == 'WAF_DisableCompilerOptimization': 
        if node_value.lower() == 'true' and node_value:
            _set_override_value(override_options_map, 'CFLAGS', env['COMPILER_FLAGS_DisableOptimization'])
            _set_override_value(override_options_map, 'CXXFLAGS', env['COMPILER_FLAGS_DisableOptimization'])                
    # [WAF_AdditionalCompilerOptions_C]
    elif node_name == 'WAF_AdditionalCompilerOptions_C':
        _set_override_value(override_options_map, 'CFLAGS', [node_value])               
    # [WAF_AdditionalCompilerOptions_CXX]
    elif node_name == 'WAF_AdditionalCompilerOptions_CXX':
        _set_override_value(override_options_map, 'CXXFLAGS', [node_value])
    # [WAF_AdditionalLinkerOptions]
    elif node_name == 'WAF_AdditionalLinkerOptions':
        _set_override_value(override_options_map, 'LINKFLAGS', node_value.split())
    # [WAF_AdditionalPreprocessorDefinitions]
    elif node_name == 'WAF_AdditionalPreprocessorDefinitions':
        _set_override_value(override_options_map, 'DEFINES', [x.strip() for x in node_value.split(';')])
    # [WAF_AdditionalIncludeDirectories]
    elif node_name == 'WAF_AdditionalIncludeDirectories':   
        path_list = [x.strip() for x in node_value.split(';')] 
        
        # Ensure paths are absolute paths
        for idx, path in enumerate(path_list):      
            if os.path.isabs(path) == False:
                path_list[idx] = proj_path.make_node(path).abspath()
        
        # Store paths
        _set_override_value(override_options_map, 'INCPATHS', path_list)            
        
###############################################################################
def collect_tasks_for_project(self):
    tasks = []
    if hasattr(self, 'compiled_tasks'):
        tasks += self.compiled_tasks
        
    if hasattr(self, 'pch_task'):
        tasks += [self.pch_task]
        
    if hasattr(self, 'link_task'):
        tasks += [self.link_task]
    return tasks    
    
###############################################################################
def collect_tasks_for_file(self, vs_file_path):
    tasks = []
    
    # look in compilation tasks 
    for t in getattr(self, 'compiled_tasks', []):           
        # [Early out] Assume a vs_file_path is unique between all task.input lists
        if tasks:
            break           
            
        for waf_path_node in t.inputs:
            # none uber_files
            if vs_file_path.lower() == waf_path_node.abspath().lower():
                tasks += [t]
                break # found match
                
            #uber files                                         
            try:
                for waf_uber_file_source_node in self.uber_file_lookup[waf_path_node.abspath()]:
                    if vs_file_path == waf_uber_file_source_node:
                        tasks += [t]
                        break # found match
            except:
                pass
    
    if hasattr(self, 'pch_task'):
        for waf_path_node in self.pch_task.inputs:
            if vs_file_path == waf_path_node.abspath():
                tasks += [t] # found match
                break # found match
        
    if hasattr(self, 'link_task'):
        for waf_path_node in self.link_task.inputs:
            if vs_file_path == waf_path_node.abspath():
                tasks += [t] # found match
                break # found match
    
    return tasks
    
###############################################################################
def get_element_name(line):     
        #e.g. <WAF_DisableCompilerOptimization> 
        posElementEnd = line.find('>',1)        
        
        # e.g. <WAF_DisableCompilerOptimization Condition"...">
        alternativeEnd = line.find(' ',1, posElementEnd)        
        if alternativeEnd != -1:
            return line[1:alternativeEnd]
            
        return line[1:posElementEnd]
        
###############################################################################
def get_element_value(line):
        pos_valueEnd = line.rfind('<')
        pos_valueStart = line[:-1].rfind('>', 0, pos_valueEnd)
        return line[pos_valueStart+1:pos_valueEnd]      


###############################################################################
# Extract per project overrides from .vcxproj file (apply to all project items)
def _get_project_overrides(ctx, target):

    if not getattr(ctx, 'is_build_cmd', False):
        return {}, {}

    if ctx.cmd == 'configure' or ctx.env['PLATFORM'] == 'project_generator':
        return ({}, {})
                
    # Only perform on VS executed builds
    if not getattr(ctx.options, 'execsolution', None):
        return ({}, {})

    if Utils.unversioned_sys_platform() != 'win32':
        return ({}, {})

    if not hasattr(ctx, 'project_options_overrides'):
        ctx.project_options_overrides = {}

    if not hasattr(ctx, 'project_file_options_overrides'):
        ctx.project_file_options_overrides = {}
        
    try:
        return (ctx.project_options_overrides[target], ctx.project_file_options_overrides[target])
    except:
        pass
    
    ctx.project_options_overrides[target] = {}
    ctx.project_file_options_overrides[target] = {}
    
    project_options_overrides = ctx.project_options_overrides[target]
    project_file_options_overrides = ctx.project_file_options_overrides[target]

    vs_spec = ctx.convert_waf_spec_to_vs_spec(ctx.options.project_spec)

    target_platform_details = ctx.get_platform_attribute(ctx.target_platform, 'msvs')
    vs_platform = target_platform_details['toolset_name']
    vs_configuration = ctx.target_configuration
    vs_valid_spec_for_build = '[%s] %s|%s' % (vs_spec, vs_configuration, vs_platform)
 
    vcxproj_file =  (ctx.get_project_output_folder(ctx.options.msvs_version).make_node('%s%s'%(target,'.vcxproj'))).abspath()

    # Example: <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='[MyProject] Profile|Win64'"> 
    project_override = '<ItemDefinitionGroup Condition="\'$(Configuration)|$(Platform)\'==\'%s\'"' % vs_valid_spec_for_build
  
    # Open file
    try:
        file = open(vcxproj_file)
    except IOError: # the only reason for this is trying to read non-existent vcxprojs, usually inapplicable to the current configuration
        return ({}, {})
    except:
        Logs.warn('warning: Unable to parse .vcxproj file to extract configuration overrides. [File:%s] [Exception:%s]' % (vcxproj_file, sys.exc_info()[0]) )
        return ({}, {})
 
    # Iterate line by line 
    #(Note: skipping lines inside loop)
    file_iter = iter(file)
    for line in file_iter:
        stripped_line = line.lstrip()
        
        #[Per Project]
        # Example:
        # <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='[MyProject] Profile|x64'">
        #   <ClCompile>
        #       <WAF_DisableCompilerOptimization>true</WAF_DisableCompilerOptimization>
        #       <WAF_AdditionalPreprocessorDefinitions>/MyProjectDefinition</WAF_AdditionalPreprocessorDefinitions>
        #   </ClCompile>
        #</ItemDefinitionGroup>     
        if stripped_line.startswith(project_override) == True:

            # Extract WAF items
            while True:
                next_line = file_iter.next().strip()
                if next_line.startswith('<WAF_'):                       
                    element_name = get_element_name(next_line)
                    element_value = get_element_value(next_line)

                    # Cache override
                    _set_override(project_options_overrides, element_name, element_value, ctx.env, ctx.path)
                    
                elif next_line.startswith('</ItemDefinitionGroup>'):
                    break
 
        #[Per Item]
        # Example:
        # <ItemGroup>
        #       <ClCompile Include="e:\P4\CE_STREAMS\Code\CryEngine\CryAnimation\Attachment.cpp">
        #           <WAF_AdditionalPreprocessorDefinitions Condition="'$(Configuration)|$(Platform)'=='[MyProject] Profile|x64'">/MyItemDefinition</WAF_AdditionalPreprocessorDefinitions>
        #           <WAF_AdditionalCompilerOptions_CXX Condition="'$(Configuration)|$(Platform)'=='[MyProject] Profile|x64'">/MyItemCompilerOption</WAF_AdditionalCompilerOptions_CXX>
        #       </ClCompile>
        # </ItemGroup>
        elif stripped_line.startswith('<ItemGroup>') == True:   
            while True:
                next_line = file_iter.next().strip()
                
                # Check that element is a "ClCompile" element that has child elements i.e. not <ClCompile ... />
                if next_line.endswith('/>') == False and next_line.startswith('<ClCompile') == True:                    
                    item_tasks = []             
                    # Is WAF Element
                    while True:
                        next_line_child = file_iter.next().strip()                      
                        if next_line_child.startswith('<WAF_'):                     
                            # Condition meets platform specs (optimize if needed)
                            if vs_valid_spec_for_build in next_line_child:
                            
                                # For first valid item, collect tasks
                                if not item_tasks:
                                    # Get include path
                                    pos_include_name_start = next_line.find('"')+1
                                    pos_include_name_end = next_line.find('"', pos_include_name_start)
                                    vs_file_path = next_line[pos_include_name_start:pos_include_name_end].lower()
                                    
                                # Get element info
                                element_name = get_element_name(next_line_child)
                                element_value = get_element_value(next_line_child)      

                                # Cache override
                                try:
                                    override_map = project_file_options_overrides[vs_file_path]
                                except:
                                    project_file_options_overrides[vs_file_path] = {}
                                    override_map = project_file_options_overrides[vs_file_path]

                                    _set_override(override_map, element_name, element_value, ctx.env, ctx.path)

                        #end of ClCompile Element
                        elif next_line_child.startswith('</ClCompile>'):
                            break
                # end of "ItemGroup" Element
                elif next_line.startswith('</ItemGroup>'):
                    break
                    
    return (project_options_overrides, project_file_options_overrides)

###############################################################################
@conf 
def get_project_overrides(ctx, target):
    return _get_project_overrides(ctx, target)[0]

###############################################################################
@conf
def get_file_overrides(ctx, target):
    return _get_project_overrides(ctx, target)[1]

###############################################################################
# Extract per project overrides from .vcxproj file (apply to all project items)
@feature('parse_vcxproj')
@before_method('verify_compiler_options_msvc')
def apply_project_compiler_overrides_msvc(self):

    if self.env['PLATFORM'] == 'project_generator':
        return
                
    # Only perform on VS executed builds
    if not hasattr(self.bld.options, 'execsolution'):
        return
    
    if not self.bld.options.execsolution:   
        return
  
    if Utils.unversioned_sys_platform() != 'win32':
        return
        
    def _apply_env_override(env, option_name, value):
        if option_name in ['CFLAGS', 'CXXFLAGS', 'LINKFLAGS', 'DEFINES', 'INCPATHS']:
            try:
                if isinstance(value, list):
                    for value_item in value:
                        if value_item not in env[option_name]:
                            env[option_name].append(value_item)
                else:
                    if value not in env[option_name]:
                        env[option_name].append(value)
            except:
                env[option_name] = value

    def _detach_and_consolidate_envs(tasks):

        cur_env = None
        for task in tasks:
            task_env = task.env
            if not compare_config_sets(task_env, cur_env, False):
                cur_env = task.env
                cur_env.detach()
                task.env = cur_env
            else:
                task.env = cur_env

    def _count_non_empty_values(item_list):
        count = 0
        for key, value in item_list.iteritems():
            if value is None:
                continue
            if isinstance(value,str):
                if len(value) > 0:
                    count += 1
            elif isinstance(value, list):
                for sub_value in value:
                    if len(sub_value) > 0:
                        count += 1
        return count


    target = getattr(self, 'base_name', None)
    if not target:
        target = self.target

    # If there are any project level overrides, process them
    project_overrides = self.bld.get_project_overrides(target)
    if len(project_overrides) > 0:

        # Scan through the overrides and make sure we have at least one non-empty item to apply
        project_override_count = _count_non_empty_values(project_overrides)
        if project_override_count > 0:

            # Apply to task task defined for the project
            project_tasks = collect_tasks_for_project(self)

            # Override only if there are any actual tasks
            if len(project_tasks) > 0:
                _detach_and_consolidate_envs(project_tasks)
                for task in project_tasks:
                    for key, value in project_overrides.iteritems():
                        _apply_env_override(task.env, key, value)

    # If there are any file overrides, process them
    file_overrides = self.bld.get_file_overrides(target)
    if len(file_overrides) > 0:

        for file_path, override_options in file_overrides.iteritems():

            # Scan through the overrides and make sure we have at least one non-empty item to apply
            file_override_count = _count_non_empty_values(override_options)
            if file_override_count > 0:

                item_tasks = collect_tasks_for_file(self, file_path)
                # Override only if there are any actual tasks
                if len(item_tasks) > 0:
                    _detach_and_consolidate_envs(item_tasks)
                    for task in item_tasks:
                        for key, value in override_options.iteritems():
                            _apply_env_override(task.env, key, value)


###############################################################################
@conf
def get_solution_overrides(self):

    if not getattr(self, 'is_build_cmd', False):
        return {}

    # Only perform on VS executed builds
    try:
        sln_file = self.options.execsolution
    except:
        return {}   
    
    if not sln_file:
            return {}   
            
    if Utils.unversioned_sys_platform() != 'win32':
        return

    # Open sln file
    try:
        file = open(sln_file)       
    except: 
        Logs.warn('warning: Unable to parse .sln file to extract configuration overrides: [File:%s] [Exception:%s]' % (sln_file, sys.exc_info()[0]) )
        return {}
        
    ret_vs_project_override = {}

    vs_spec = self.convert_waf_spec_to_vs_spec(self.options.project_spec)

    target_platform_details = self.get_platform_attribute(self.target_platform, 'msvs')
    vs_platform = target_platform_details['toolset_name']
    vs_configuration = self.target_configuration
    
    vs_build_configuration = '[%s] %s|%s' % (vs_spec, vs_configuration, vs_platform.split()[0]) # Example: [MyProject] Debug|x64
    vs_project_identifier = 'Project("{8BC9CEB8' # full project id ... Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}")
    
    # Iterate over all basic project  information
    # Example:
    #   Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "Cry3DEngine", "e:\P4\CE_STREAMS\Solutions\.depproj\Cry3DEngine.vcxproj", "{60178AE3-57FD-488C-9A53-4AE4F66419AA}"
    project_guid_to_name = {}
    file_iter = iter(file)
    for line in file_iter:
        stripped_line = line.lstrip()   
        if stripped_line.startswith(vs_project_identifier) == True:
            
            project_info = stripped_line[51:].split(',')  # skip first 51 character ... "Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") ="
            project_name = project_info[0].strip()[1:-1]  # trim left and right and remove '"'
            project_path = project_info[1].strip()[1:-1]
            project_guid = project_info[2].strip()[2:-2]
                        
            # Store project GUID and Name pair
            project_guid_to_name[project_guid] = project_name
            
        elif stripped_line.startswith('Global') == True:
            file_iter.next()
            break 
        else:
            continue
            
    # Skip to beginning of project configurations information
    for line in file_iter:      
        if line.lstrip().startswith('GlobalSection(ProjectConfigurationPlatforms) = postSolution') == True:
            file_iter.next()
            break       
      
    # Loop over all project
    # Example:
    # {60178AE3-57FD-488C-9A53-4AE4F66419AA}.[MyProject] Debug|x64.ActiveCfg = [Game and Tools] Debug|x64
    # or 
    # {60178AE3-57FD-488C-9A53-4AE4F66419AA}.[MyProject] Debug|x64.Build.0 = [Game and Tools] Debug|x64
    for line in file_iter:
        stripped_line = line.strip()
        
        # Reached end of section
        if stripped_line.startswith('EndGlobalSection'):
            break       
        
        project_build_info = stripped_line.split('.')
        if (len(project_build_info) == 3) and (project_build_info[1] == vs_build_configuration):
            starts_of_override_configuration = project_build_info[-1].find('[')
            project_build_info[-1] = project_build_info[-1][starts_of_override_configuration:] # remove anything prior to [xxx] e.g. "ActiveCfg = " 
            
            vs_project_configuration = project_build_info[1]
            vs_project_override_configuation = project_build_info[-1]

            # Check for no override condition
            if vs_project_configuration == vs_project_override_configuation:
                continue
                            
            project_guid = project_build_info[0][1:-1] # remove surrounding '{' and '}'
            project_name = project_guid_to_name[project_guid]
            
            # Check that spec is the same
            vs_override_spec_end = vs_project_override_configuation.find(']')
            vs_override_spec = vs_project_override_configuation[1:vs_override_spec_end]
            if vs_spec != vs_override_spec:
                self.cry_error('Project "%s" : Invalid override spec is of type "%s" when it should be "%s"' % (project_name, vs_override_spec, vs_spec))
            
            # Get WAF configuration from VS project configuration e.g. [MyProject] Debug|x64 -> debug
            vs_project_configuration_end = vs_project_override_configuation.rfind('|')
            vs_project_configuration_start = vs_project_override_configuation.rfind(']', 0, vs_project_configuration_end) + 2
            waf_configuration = vs_project_override_configuation[vs_project_configuration_start : vs_project_configuration_end]

            # Store override
            ret_vs_project_override[project_name] = waf_configuration
            Logs.info("MSVS: User has selected %s for %s in visual studio.  Overriding for this build." % (waf_configuration, project_name))
            
    return ret_vs_project_override
