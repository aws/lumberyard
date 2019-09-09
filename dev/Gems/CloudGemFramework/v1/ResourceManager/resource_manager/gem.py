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
# $Revision$

from __future__ import print_function

# Python modules...
import json
import os, stat
import shutil
import subprocess
import uuid
import sys

# Resource manager modules...
from resource_manager_common import constant
import file_util
import util
from errors import HandledError
from cgf_utils.version_utils import Version
from resource_group import ResourceGroup

class GemContext(object):


    def __init__(self, context):
        self.__context = context
        self.__enabled_gems = None
        self.__gems = {}
        self.__lmbr_exe_path = None
        self.__initial_gem_content_list = None
        self.__verbose = False
        #needed to allow the config bootstrap to initialize since it requires the enabled gems
        self.__explicit_cloud_gems = []

    def bootstrap(self, args):
        self.__verbose = args.verbose
        self.__explicit_cloud_gems = args.only_cloud_gems 
        gem_id = "CloudGemFramework"
        if self.__explicit_cloud_gems:
            self.__explicit_cloud_gems.append(gem_id)
        
        #reset the enabled gems as all gems are added on the init due to the local projects setting initialization occuring before the bootstrap of the GemContext
        self.__enabled_gems = None
        #Initialize the resource groups based on the gems defined
        for gem in self.enabled_gems:                        
            self.__add_gem_to_resource_groups(gem)         

    @property
    def initial_content_list(self):
        if self.__initial_gem_content_list is None:
            self.__initial_gem_content_list =  os.listdir(os.path.join(self.__context.config.resource_manager_path, 'initial-gem-content'))
        return self.__initial_gem_content_list
 

    def create_gem(
        self, 
        gem_name, 
        initial_content = 'empty', 
        enable = False, 
        asset_only = False, 
        version = '1.0.0', 
        relative_directory_path = None,
        lmbr_exe_path_override = None,
        no_sln_change = False):

        # Validate arguments

        try:
            util.validate_stack_name(gem_name)
        except HandledError as e:
            raise HandledError('The gem name is not valid for use as a Cloud Gem. {}.'.format(e))

        if initial_content not in self.initial_content_list:
            raise HandledError('Invalid initial content: {}. Valid initial content values are {}.'.format(initial_content, ', '.join(self.initial_content_list)))

        # Validate files are writable

        writable_file_list = []

        if enable:
            writable_file_list.append(self.get_gems_file_path())

        if writable_file_list:
            if not util.validate_writable_list(self.__context, writable_file_list):
                return

        # Create the gem...

        root_directory_path = self.__do_lmbr_gems_create(lmbr_exe_path_override, gem_name, relative_directory_path, asset_only, version)

        # Add the framework dependency
        
        self.__add_framework_gem_dependency(root_directory_path)

        # Copy initial gem content...

        self.__copy_initial_gem_content(gem_name, root_directory_path, initial_content, no_sln_change)

        # Set the c++ build configuration for the gem.

        if not asset_only:
            self.__setup_cloud_gem_cpp_build(root_directory_path, gem_name, initial_content) 

        # Add gem to collections...

        gem = self.__add_gem(root_directory_path, enable)

        # Tell them about it, then go on to enable if requested...

        self.__context.view.gem_created(gem_name, root_directory_path)

        if enable:
            self.enable_gem(gem_name, lmbr_exe_path_override)


    def __do_lmbr_gems_create(self, lmbr_exe_path_override, gem_name, relative_directory_path, asset_only, version):
        gems_file_content = self.__get_gems_file_content()
        gem_names = []
        for gem in gems_file_content.get('Gems', []):
            if gem.get('_comment', ''):
                gem_names.append(gem['_comment'])
        if gem_name in gem_names:
            raise HandledError('Cloud gems named {} exists in the project. Cloud gems must have unique names within a project.'.format(gem_name))
         

        if relative_directory_path is None:
            relative_directory_path = os.path.join(
                gem_name,
                'v' + str(Version(version).major))

        full_directory_path = os.path.join(
                self.__context.config.root_directory_path,
                'Gems',
                relative_directory_path)
             
        lmbr_exe_path = self.__get_lmbr_exe_path(lmbr_exe_path_override)
        args = [lmbr_exe_path, 'gems', 'create', gem_name, '-version', version, '-out-folder', relative_directory_path]
        if asset_only:
            args.append('-asset-only')

        try:
            self.__execute(args)
        except Exception as e:
            raise HandledError('Gem creation failed. {}'.format(e.message))

        return full_directory_path


    def __add_framework_gem_dependency(self, root_directory_path):

        gem_file_path = os.path.join(root_directory_path, 'gem.json')

        with open(gem_file_path, 'r') as file:
            gem_file_content = json.load(file)

        dependencies = gem_file_content.setdefault('Dependencies', [])
        dependencies.append(
            {
                "Uuid": "6fc787a982184217a5a553ca24676cfa",
                "VersionConstraints": [ "~>" + str(self.__context.config.framework_version) ],
                "_comment": "CloudGemFramework"
            }
        )

        with open(gem_file_path, 'w') as file:
            json.dump(gem_file_content, file)


    def __copy_initial_gem_content(self, gem_name, root_directory_path, initial_content, no_sln_change):

        self.__context.view.copying_initial_gem_content(initial_content)

        src_content_path = os.path.join(self.__context.config.resource_manager_path, 'initial-gem-content', initial_content)
        dst_content_path = os.path.join(root_directory_path, 'AWS')
        relative_path = root_directory_path.replace(self.__context.config.gem_directory_path, '')
        relative_returns = ["../"] * (len(relative_path.split(os.path.sep)) + 2)
        
        name_substituions = content_substitutions = {
            '$-GEM-NAME-$': gem_name,
            '$-GEM-NAME-LOWER-CASE-$': gem_name.lower(),
            '$-PROJECT-GUID-$': str(uuid.uuid4()),
            '$-RELATIVE-RETURNS-$': ''.join(relative_returns)
        }
        
        if not no_sln_change:
            self.__copy_project_to_solution(content_substitutions, src_content_path, os.path.join(self.__context.config.gem_directory_path, "CloudGemFramework", "v1", "Website", "CloudGemPortal.sln"), '..\..\..{0}\AWS\cgp-resource-code\src'.format(relative_path), '.njsproj')        

        file_util.copy_directory_content(
            self.__context, 
            dst_content_path, 
            src_content_path, 
            overwrite_existing = False, 
            name_substituions = name_substituions,
            content_substitutions = content_substitutions
            )

    def __copy_project_to_solution(self, content_substitutions, src_content_path, src_sln, dest, ext):                
        for root, subdirs, files in os.walk(src_content_path):
            for fname in os.listdir(root):                        
                if fname.endswith(ext):         
                    #do not modify the UUID's       
                    project_entry = '\nProject("{{9092AA53-FB77-4645-B42D-1CCCA6BD08BD}}") = "{0}", "{2}\{0}{3}", "{1}"\nEndProject\n'.format(content_substitutions.get('$-GEM-NAME-$'), "{"+content_substitutions.get('$-PROJECT-GUID-$')+"}", dest, ext)                                        
                    backup_sln = src_sln+".bak"
                    #write the sln to a local backup
                    shutil.copyfile(src_sln, backup_sln)
                    #remove the readonly flag if it is set
                    os.chmod( src_sln, stat.S_IWRITE )                    
                    with open(src_sln, "a") as solution:
                        solution.write(project_entry)

    def __setup_cloud_gem_cpp_build(self, root_directory_path, gem_name, initial_content):

        aws_directory_path = os.path.join(root_directory_path, constant.GEM_AWS_DIRECTORY_NAME)

        # If there is a swagger.json file, generate service api client code.

        swagger_file_path = os.path.join(aws_directory_path, 'swagger.json')
        if os.path.exists(swagger_file_path):

            args = [
                self.__lmbr_aws_path, 
                'cloud-gem-framework', 
                'generate-service-api-code', 
                '--gem', root_directory_path, 
                '--update-waf-files']

            try:
                self.__execute(args, use_shell=True)
            except Exception as e:
                raise HandledError('Could not generate initial client code for service api. {}'.format(e.message))
        
        # Use the initial-content option to determine what additional AWS SDK libs are needed.
        # We don't expect game clients to directly access most AWS services (TODO: game 
        # servers). 
        #
        # The resource-manager-plugin initial content doesn't require any AWS API support, so
        # we leave the wscript alone in that case. Initial content starting with 'api' only
        # needs AWS SDK Core.
        #
        # Unfortuantly we can't easly modify the origional wscript file. Since it was just
        # created it should still have only the default, we just overwrite it. 
        #
        # We also write an aws_unsupported.waf_files, which is specified by the wscript for
        # platforms that do not support AWS. This includes onl the minimal files needed to
        # get the gem to build.

        if initial_content != 'resource-manager-plugin':

            use_lib_list = [ 'AWS_CPP_SDK_CORE' ]
            if initial_content.startswith('lambda'):
                use_lib_list.append('AWS_CPP_SDK_LAMBDA')
            elif initial_content.startswith('bucket'):
                use_lib_list.append('AWS_CPP_SDK_S3')

            wscript_file_path = os.path.join(root_directory_path, 'Code', 'wscript')
            self.__context.view.saving_file(wscript_file_path)
            with open(wscript_file_path, 'w') as file:
                wscript_content = CLOUD_GEM_WSCRIPT_FILE_CONTENT
                wscript_content = wscript_content.replace('$-GEM-NAME-LOWER-CASE-$', gem_name.lower())
                wscript_content = wscript_content.replace('$-USE-LIB-LIST-$', str(use_lib_list))
                file.write(wscript_content)

            unsupported_waf_files_file_path = os.path.join(root_directory_path, 'Code', 'aws_unsupported.waf_files')
            self.__context.view.saving_file(unsupported_waf_files_file_path)
            with open(unsupported_waf_files_file_path, 'w') as file:
                aws_unsupported_file_content = AWS_UNSUPPORTED_WAF_FILES_FILE_CONTENT.replace('$-GEM-NAME-$', gem_name)
                file.write(aws_unsupported_file_content)

            gem_file_path = os.path.join(root_directory_path, 'gem.json')
            gem_file_content = util.load_json(gem_file_path, optional = False)
            gem_uuid = gem_file_content.get('Uuid', None)
            if gem_uuid is None:
                raise HandledError('Missing required Uuid property in {}.'.format(gem_file_path))

            component_stub_cpp_file_path = os.path.join(root_directory_path, 'Code', 'Source', 'ComponentStub.cpp')
            self.__context.view.saving_file(component_stub_cpp_file_path)
            with open(component_stub_cpp_file_path, 'w') as file:
                component_stub_cpp_file_content = COMPONENT_STUB_CPP_FILE_CONTENT.replace('$-GEM-NAME-$', gem_name)
                component_stub_cpp_file_content = COMPONENT_STUB_CPP_FILE_CONTENT.replace('$-GEM-UUID-$', gem_uuid)
                file.write(component_stub_cpp_file_content)


    def enable_gem(self, gem_name, lmbr_exe_path_override = None):
        # verify files are writable

        if not util.validate_writable_list(self.__context, [ self.get_gems_file_path() ]):
            return False

        # call lmbr.exe to enable the gem

        lmbr_exe_path = self.__get_lmbr_exe_path(lmbr_exe_path_override)

        args = [lmbr_exe_path, 'gems', 'enable', self.__context.config.game_directory_name, gem_name]
        try:
            self.__execute(args)
        except Exception as e:
            raise HandledError('Gem enable failed. {}'.format(e.message))

        # add the gem to list of gems, and load as resource group if needed
        gems_file_content = self.__get_gems_file_content()
        for gem_file_entry in gems_file_content.get('Gems', []):
            if gem_file_entry.get('_comment') == gem_name :
                directory_path = self.__get_gem_root_directory_path_from_gems_file_entry(gem_file_entry)
                gem = self.__add_gem(directory_path, is_enabled = True)
                self.__add_gem_to_resource_groups(gem)
                break

        # all done
        self.__context.view.gem_enabled(gem_name)


    def disable_gem(self, gem_name, lmbr_exe_path_override = None):

        # verify files are writable

        if not util.validate_writable_list(self.__context, [ self.get_gems_file_path() ]):
            return False

        # call lmbr.exe to disable the gem

        lmbr_exe_path = self.__get_lmbr_exe_path(lmbr_exe_path_override)

        args = [self.__lmbr_exe_path, 'gems', 'disable', self.__context.config.game_directory_name, gem_name]
        try:
            self.__execute(args)
        except Exception as e:
            raise HandledError('Gem {} disable failed.'.format(gem_name))

        # remove from gem list and resource group list, if needed

        gem = self.get_by_name(gem_name)
        if gem in self.__enabled_gems:
            self.__enabled_gems.remove(gem)

        self.__context.resource_groups.remove_resource_group(gem)

        # all done

        self.__context.view.gem_disabled(gem_name)

    def __add_gem_to_resource_groups(self, gem):        
        if gem and gem.has_aws_file(constant.RESOURCE_GROUP_TEMPLATE_FILENAME):                 
            gem_resource_group = ResourceGroup(self.__context, gem.name, gem.aws_directory_path, gem.cpp_base_directory_path, gem.cpp_aws_directory_path)        
            self.__context.resource_groups.add_resource_group(gem_resource_group)        

    def __execute(self, args, use_shell=False):

        self.__context.view.executing_subprocess(args)

        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW

        popen = subprocess.Popen(
            args, 
            stdout=subprocess.PIPE,   # Pipe stdout to the loop below
            stderr=subprocess.STDOUT, # Redirect stderr to stdout
            stdin=subprocess.PIPE,    # See below.
            universal_newlines=True,  # Convert CR/LF to LF
            shell = use_shell,
            startupinfo = startupinfo
        )

        # Piping stdin and closing it causes the process to exit if it attempts to read input.
        popen.stdin.close() 

        # Read output from the process one line at a time. We echo this output and save it for
        # the error message should the command fail.
        output_lines = []
        for output_line in iter(popen.stdout.readline, ""):
            if self.__verbose:
                print(output_line)
            output_lines.append(output_line)

        # After the iterator finishes, close the pipe and get the process exit code.
        popen.stdout.close()
        exit_code = popen.wait()

        # If the process failed, raise an exception.
        if exit_code:
            raise RuntimeError('\n'.join(output_lines))

        if self.__verbose:
            self.__context.view.executed_subprocess(args)


    @property
    def enabled_gems(self):
        self.__load_enabled_gems()
        return self.__enabled_gems


    def __load_enabled_gems(self, version = 'latest'):
        if self.__enabled_gems:
            return

        self.__enabled_gems = []

        gems_file = self.__get_gems_file_content()
        if gems_file:
            for gems_file_entry in gems_file.get('Gems',[]):
                root_directory_path = self.__get_gem_root_directory_path_from_gems_file_entry(gems_file_entry)
                self.__add_gem(root_directory_path, is_enabled = True)
        else:
            # fail gracefully in this case - tests do this
            self.__context.view.missing_gems_file(self.get_gems_file_path())


    def __get_gem_root_directory_path_from_gems_file_entry(self, gems_file_entry):
        relative_path = gems_file_entry.get('Path', None)
        if relative_path is None:
            raise HandledError('No Path property provided for {} in {}.'.format(gems_file_object, self.get_gems_file_path()))
        return os.path.abspath(os.path.join(self.__context.config.root_directory_path, relative_path.replace('/', os.sep)))


    def __get_gems_file_content(self):
        gems_file_path = self.get_gems_file_path()
        return self.__context.config.load_json(gems_file_path)


    def get_gems_file_path(self):
        return os.path.join(self.__context.config.game_directory_path, constant.PROJECT_GEMS_DEFINITION_FILENAME)


    def __add_gem(self, root_directory_path, is_enabled):
        gem = self.__get_loaded_gem_by_root_directory_path(root_directory_path, is_enabled)
        # check to see if the user only wants to act against a specific set of cloud gems            
        is_cloud_gem_allowed = gem and gem.name in self.__explicit_cloud_gems if self.__explicit_cloud_gems else True
        
        if gem is not None and is_enabled and gem not in self.__enabled_gems and is_cloud_gem_allowed:
            self.__enabled_gems.append(gem) 
        


    def get_by_root_directory_path(self, root_directory_path):
        self.__load_enabled_gems()
        return self.__get_loaded_gem_by_root_directory_path(root_directory_path, is_enabled = False)


    def __get_loaded_gem_by_root_directory_path(self, root_directory_path, is_enabled):
        gem = self.__gems.get(root_directory_path, None)
        if gem is None:
            gem_file_path = os.path.join(root_directory_path, 'gem.json')
            if not os.path.exists(gem_file_path):
                return None
            gem = Gem(self.__context, root_directory_path, is_enabled = is_enabled)
            self.__gems[root_directory_path] = gem
        return gem


    def __get_lmbr_exe_path(self, override):
        if override:
            if not os.path.isfile(override):
                raise HandledError('lmbr.exe not found at {}.'.format(override))
            return override
        else:
            if self.__lmbr_exe_path is None:
                self.__lmbr_exe_path = self.__get_default_lmbr_exe_path()
            return self.__lmbr_exe_path

    @staticmethod
    def get_lmbr_exe_path_from_root(root_path):

        directory_names = []
        # lmbr.exe used to live in Tools/LmbrSetup/{platform].{compiler}(.Debug)(.Test)
        baselmbrpath = os.path.join('Tools', 'LmbrSetup')

        for platformname in ['Win','Win.vc120','Win.vc140','Win.vc141','Mac','Mac.clang','Linux','Linux.clang','Linux.gcc']:
            directory_names.append(os.path.join(baselmbrpath,'{}.Debug.Test'.format(platformname)))
            directory_names.append(os.path.join(baselmbrpath,'{}'.format(platformname)))
            directory_names.append(os.path.join(baselmbrpath,'{}.Debug'.format(platformname)))

        if sys.platform == 'win32':
            folder_list = ["Bin64vc140", "Bin64vc141"]
            lmbr_name = 'lmbr.exe'
        elif sys.platform == 'darwin':
            folder_list = ["BinMac64"]
            lmbr_name = 'lmbr'
        # lmbr.exe was moved back to normal Bin directory
        for bin_folder_name in folder_list:
            directory_names.append(os.path.join(
                root_path, '{}.Debug.Test'.format(bin_folder_name)))
            directory_names.append(os.path.join(
                root_path, '{}.Debug'.format(bin_folder_name)))
            directory_names.append(os.path.join(
                root_path, '{}'.format(bin_folder_name)))

        newest_timestamp = None
        newest_path = None

        for directory_name in directory_names:
            path = os.path.join(root_path, directory_name, lmbr_name)

            if os.path.isfile(path):
                if newest_path is None:
                    newest_path = path
                    newest_timestamp = os.path.getmtime(path)
                else:
                    timestamp = os.path.getmtime(path)
                    if timestamp > newest_timestamp:
                        newest_timestamp = timestamp
                        newest_path = path


        if newest_path is None:
            raise HandledError('Could not find lmbr.exe in any of the following subirectories of {}: {}. Please use lmbr_waf to build lmbr.exe.'.format(
                root_path,
                ', '.join(directory_names)))

        return newest_path


    def __get_default_lmbr_exe_path(self):
        return GemContext.get_lmbr_exe_path_from_root(self.__context.config.root_directory_path)

    @property
    def __lmbr_aws_path(self):
        return os.path.join(self.__context.config.root_directory_path, 'lmbr_aws.cmd')

    def get_by_name(self, gem_name):
        self.__load_enabled_gems()
        for gem in self.__enabled_gems:
            if gem.name == gem_name:
                return gem
        return None


    @property
    def framework_gem(self):
        framework_gem = self.get_by_name('CloudGemFramework')
        if framework_gem is None:
            raise HandledError('The CloudGemFramework gem must be enabled for the project.')
        return framework_gem


class Gem(object):

    def __init__(self, context, root_directory_path, is_enabled):
        self.__context = context
        self.__root_directory_path = root_directory_path
        self.__is_enabled = is_enabled
        self.__aws_directory_path = os.path.join(self.__root_directory_path, constant.GEM_AWS_DIRECTORY_NAME)
        self.__gem_file_path = os.path.join(self.__root_directory_path, constant.GEM_DEFINITION_FILENAME)
        self.__gem_file_object_ = None
        self.__cli_plugin_code_path = os.path.join(self.__aws_directory_path, 'cli-plugin-code')
        self.__cgp_resource_code_paths = os.path.join(self.__aws_directory_path, constant.GEM_CGP_DIRECTORY_NAME)
        self.__version = None
    
    @property
    def __gem_file_object(self):
        if self.__gem_file_object_ is None:
            if not os.path.isfile(self.__gem_file_path):
                raise HandledError('The {} directory contains no gem.json file.'.format(self.__root_directory_path))
            self.__gem_file_object_ = self.__context.config.load_json(self.__gem_file_path)
        return self.__gem_file_object_

    @property
    def is_enabled(self):
        return self.__is_enabled

    @property
    def is_defined(self):
        return os.path.isfile(self.__gem_file_path)

    @property
    def name(self):
        name = self.__gem_file_object.get('Name', None)
        if name is None:
            raise HandledError('No Name property found in the gem.json file at {}'.format(self.__root_directory_path))
        return name

    @property
    def version(self):
        if self.__version is None:
            self.__version = Version(self.__gem_file_object.get('Version', '0.0.0'))
        return self.__version

    @property
    def display_name(self):
        display_name = self.__gem_file_object.get('DisplayName', None)
        if display_name is None:
            display_name = self.name
        return display_name

    @property
    def root_directory_path(self):
        return self.__root_directory_path

    @property
    def relative_path(self):
        return self.__relative_path

    @property
    def aws_directory_path(self):
        return self.__aws_directory_path

    def has_aws_file(self, *args):
        return os.path.isfile(os.path.join(self.aws_directory_path, *args))

    def has_aws_directory(self, *args):
        return os.path.isdirectory(os.path.join(self.aws_directory_path, *args))

    @property
    def aws_directory_exists(self):
        return os.path.isdir(self.__aws_directory_path)

    @property
    def cpp_aws_directory_path(self):
        return os.path.join(self.cpp_base_directory_path, constant.GEM_AWS_DIRECTORY_NAME)

    @property
    def cpp_base_directory_path(self):
        return os.path.join(self.root_directory_path, constant.GEM_CODE_DIRECTORY_NAME)

    @property
    def cli_plugin_code_path(self):
        return self.__cli_plugin_code_path

    @property
    def cgp_resource_code_path(self):
        return self.__cgp_resource_code_path

    @property
    def file_object(self):
        return self.__gem_file_object

    @property
    def uuid(self):
        return self.__gem_file_object['Uuid'] if 'Uuid' in self.__gem_file_object else None


def add_gem_cli_commands(context, subparsers, add_common_args):

    subparser = subparsers.add_parser('cloud-gem', aliases=['gem'], help='Perform gem operations')

    subparsers = subparser.add_subparsers(dest='subparser_name', metavar='COMMAND')

    subparser = subparsers.add_parser('create', help='Create a new Cloud Gem.')
    subparser.add_argument('--gem', '-g', dest='gem_name', required=True, metavar='GEM', help='The name of the gem to create.')
    subparser.add_argument('--initial-content',  '-i', dest='initial_content', required=False, default='no-resources', metavar='CONTENT',
        help='Initialize the Cloud Gem with one the following configurations: {}. The default is "no-resources", which defines no AWS resources or plugins.'.format(
            ', '.join(context.gem.initial_content_list)))
    subparser.add_argument('--enable', '-e', dest='enable', required=False, default=None, action='store_true', help='Enables the gem for the current project.')
    subparser.add_argument('--no-cpp-code', dest='asset_only', required=False, action='store_true', help='Defines a gem that contains no C++ code (it does not build as a dll). The gem can contain resource group definitions and/or resource manager plugins.')
    subparser.add_argument('--version', dest='version', required=False, default='1.0.0', metavar='VERSION', help='Specifies the initial version for the gem. Defaults to 1.0.0.')
    subparser.add_argument('--directory', dest='relative_directory_path', required=False, default=None, metavar='PATH', help='The location in the Gems directory where the gem will be created. The default is ...\dev\Gems\NAME\vMAJOR, where NAME was specified by the --gem option and vMAJOR is the major part of the gem\'s version as specified by the --version option.')
    subparser.add_argument('--lmbr-exe', dest='lmbr_exe_path_override', required=False, default=None, metavar='PATH', help='The path to the lmbr.exe executable that will be used. The default is the most recently built executable.')
    subparser.add_argument('--no-sln-change', dest='no_sln_change', required=False, default=False, action='store_true', help='Defines whether the solution files should not be touched when creating this gem.  By default the project solution files add the new gem project.')
    add_common_args(subparser)
    subparser.set_defaults(context_func=context.gem.create_gem)

    subparser = subparsers.add_parser('enable', help='Enable a Cloud Gem for the current project.')
    subparser.add_argument('--gem', '-g', dest='gem_name', required=True, metavar='GEM', help='The name of the gem to enable for the current project.')
    subparser.add_argument('--lmbr-exe', dest='lmbr_exe_path_override', required=False, default=None, metavar='PATH', help='The path to the lmbr.exe executable that will be used. The default is the most recently built executable.')
    add_common_args(subparser)
    subparser.set_defaults(context_func=context.gem.enable_gem)

    subparser = subparsers.add_parser('disable', help='Disable a Cloud Gem in the current project.')
    subparser.add_argument('--gem', '-g', dest='gem_name', required=True, metavar='GEM', help='The name of the gem to disable in the current project.')
    subparser.add_argument('--lmbr-exe', dest='lmbr_exe_path_override', required=False, default=None, metavar='PATH', help='The path to the lmbr.exe executable that will be used. The default is the most recently built executable.')
    add_common_args(subparser)
    subparser.set_defaults(context_func=context.gem.disable_gem)


def create_gem(context, args):
    context.gem.create_gem(args.gem_name, initial_content=args.initial_content, enable=True)


CLOUD_GEM_WSCRIPT_FILE_CONTENT = '''
def build(bld):
    import lumberyard_sdks

    file_list = []
    if bld.env['PLATFORM'] == 'project_generator':
        file_list.append('$-GEM-NAME-LOWER-CASE-$.waf_files')
        file_list.append('aws_unsupported.waf_files')
    else:
        if lumberyard_sdks.does_platform_support_aws_native_sdk(bld):
            file_list.append('$-GEM-NAME-LOWER-CASE-$.waf_files')
        else:
            file_list.append('aws_unsupported.waf_files')

    bld.DefineGem(
        
        disable_pch = True,

        file_list = file_list,

        platforms = ['all'],
        uselib = $-USE-LIB-LIST-$,

        use = ['CloudGemFramework.StaticLibrary']

    )
'''

AWS_UNSUPPORTED_WAF_FILES_FILE_CONTENT = '''
{
    "auto": {
        "Implementation": [
            "Source/ComponentStub.cpp"
        ]
    }
}
'''

COMPONENT_STUB_CPP_FILE_CONTENT = '''
#include <AzCore/Module/Module.h>

AZ_DECLARE_MODULE_CLASS($-GEM-NAME-$_$-GEM-UUID-$, AZ::Module)
'''
