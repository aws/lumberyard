#! /usr/bin/env python
# encoding: utf-8

# Modifications copyright Amazon.com, Inc. or its affiliates.

# XCode 3/XCode 4 generator for Waf
# Nicolas Mercier 2011
# Christopher Bolte: Created a copy to easier adjustment to crytek specific changes

"""
Usage:

def options(opt):
    opt.load('xcode')

$ waf configure xcode_ios
$ waf configure xcode_mac
$ waf configure xcode_appletv
"""

from waflib import Context, TaskGen, Build, Utils, Logs, Options
from waflib.TaskGen import feature, after_method
from waflib.Configure import conf, conf_event, ConfigurationContext

from build_configurations import PLATFORM_MAP

import copy
import os
import re
import subprocess
import sys

HEADERS_GLOB = '**/(*.h|*.hpp|*.H|*.inl)'

MAP_EXT = {
    '.h' :  "sourcecode.c.h",

    '.hh':  "sourcecode.cpp.h",
    '.inl': "sourcecode.cpp.h",
    '.hpp': "sourcecode.cpp.h",

    '.c':   "sourcecode.c.c",

    '.m':   "sourcecode.c.objc",

    '.mm':  "sourcecode.cpp.objcpp",

    '.cc':  "sourcecode.cpp.cpp",

    '.cpp': "sourcecode.cpp.cpp",
    '.C':   "sourcecode.cpp.cpp",
    '.cxx': "sourcecode.cpp.cpp",
    '.c++': "sourcecode.cpp.cpp",

    '.l':   "sourcecode.lex", # luthor
    '.ll':  "sourcecode.lex",

    '.y':   "sourcecode.yacc",
    '.yy':  "sourcecode.yacc",

    '.plist': "text.plist.xml",
    '.nib':   "wrapper.nib",
    '.xib':   "text.xib",
}

LMBR_WAF_SCRIPT_REL_PATH = 'Tools/build/waf-1.7.13/lmbr_waf'

XCTEST_WRAPPER_TARGETS = ('ios', )
XCTEST_WRAPPER_REL_PATH = 'Code/Launcher/AppleLaunchersCommon/XCTestWrapper'
XCTEST_WRAPPER_SOURCE = [
    'LumberyardXCTestWrapperTests.mm',
]
XCTEST_WRAPPER_FILES = XCTEST_WRAPPER_SOURCE + [ 'Info.plist' ]

PLATFORM_SDK_NAME = {
    'appletv'       : 'appletvos',
    'darwin_x64'    : 'macosx',
    'ios'           : 'iphoneos',
}

FRAMEWORKS_REL_PATH = 'System/Library/Frameworks'

root_dir = ''
id = 562000999
uintmax = 2147483647

def newid():
    global id
    id = id + 1
    return "%04X%04X%04X%012d" % (0, 10000, 0, id)

def XCodeEscapeSpacesForShell(spaceString):
    return spaceString.replace(' ', '\\ ')

class XCodeNode:
    def __init__(self):
        self._id = newid()

    def tostring(self, value):
        if isinstance(value, dict):
            result = "{\n"
            for k,v in value.items():
                result = result + "\t\t\t%s = %s;\n" % (k, self.tostring(v))
            result = result + "\t\t}"
            return result
        elif isinstance(value, str):
            return "\"%s\"" % value
        elif isinstance(value, list):
            result = "(\n"
            for i in value:
                result = result + "\t\t\t%s,\n" % self.tostring(i)
            result = result + "\t\t)"
            return result
        elif isinstance(value, XCodeNode):
            return value._id
        else:
            return str(value)

    def write_recursive(self, value, file):
        if isinstance(value, dict):
            for k,v in value.items():
                self.write_recursive(v, file)
        elif isinstance(value, list):
            for i in value:
                self.write_recursive(i, file)
        elif isinstance(value, XCodeNode):
            value.write(file)

    def write(self, file):
        for attribute,value in self.__dict__.items():
            if attribute[0] != '_':
                self.write_recursive(value, file)

        w = file.write
        w("\t%s = {\n" % self._id)
        w("\t\tisa = %s;\n" % self.__class__.__name__)
        for attribute,value in self.__dict__.items():
            if attribute[0] != '_':
                w("\t\t%s = %s;\n" % (attribute, self.tostring(value)))
        w("\t};\n\n")


# Configurations
class XCBuildConfiguration(XCodeNode):
    def __init__(self, name, settings = {}):
        XCodeNode.__init__(self)
        self.baseConfigurationReference = ""
        self.buildSettings = settings
        self.name = name


class XCConfigurationList(XCodeNode):
    def __init__(self, settings):
        XCodeNode.__init__(self)
        self.buildConfigurations = settings
        self.defaultConfigurationIsVisible = 0
        self.defaultConfigurationName = settings and settings[0].name or ""


# Group/Files
class PBXFileReference(XCodeNode):
    def __init__(self, name, path, filetype = '', sourcetree = "SOURCE_ROOT", explicit_filetype = ''):
        XCodeNode.__init__(self)
        self.fileEncoding = 4
        if not filetype:
            _, ext = os.path.splitext(name)
            filetype = MAP_EXT.get(ext, 'text')
        self.lastKnownFileType = filetype
        self.name = name
        self.path = path
        self.sourceTree = sourcetree
        if explicit_filetype:
            self.explicitFileType = explicit_filetype

class PBXGroup(XCodeNode):
    folders_cache = {}

    def __init__(self, name, sourcetree = "<group>"):
        XCodeNode.__init__(self)
        self.children = []
        self.name = name
        self.sourceTree = sourcetree

    def sort_recursive(self):
        self.children.sort(key=lambda child: [not isinstance(child, PBXGroup), child.name])
        for child in self.children:
            if isinstance(child, PBXGroup):
                child.sort_recursive()

    def add(self, root, sources):
        folders = PBXGroup.folders_cache
        def folder(node):
            if not node:
                return self

            Logs.debug('xcode_group: %s', node)
            Logs.debug('xcode_group: %s', node.name)
            Logs.debug('xcode_group: %s', node.parent)

            if node == root:
                return self

            try:
                return folders[node]
            except KeyError:
                new_folder = PBXGroup(node.name)
                parent = folder(node.parent)
                folders[node] = new_folder
                parent.children.append(new_folder)
                return new_folder

        for s in sources:
            Logs.debug('xcode_group: ========')
            Logs.debug('xcode_group: %s', s)

            a_folder = folder(s.parent)
            source = PBXFileReference(s.name, s.abspath())
            a_folder.children.append(source)

    def get_child_file_references_recursive(self, file_references):
        for child in self.children:
            if isinstance(child, PBXGroup):
                child.get_child_file_references_recursive(file_references)
            elif isinstance(child, PBXFileReference):
                file_references.append(child)


# Targets
class PBXLegacyTarget(XCodeNode):
    def __init__(self, platform_name, project_spec, target, ctx):
        XCodeNode.__init__(self)
        global root_dir

        build_configuration_list = []
        for config in ctx.get_target_configurations():
            build_configuration_list.append(XCBuildConfiguration(config))
        self.buildConfigurationList = XCConfigurationList(build_configuration_list)

        action = 'build_' + platform_name + "_$CONFIGURATION"
        lmbr_waf_script = os.path.join(ctx.engine_path, LMBR_WAF_SCRIPT_REL_PATH)
        self.buildArgumentsString = "%s -cwd %s %s -p%s --targets=%s" % (XCodeEscapeSpacesForShell(lmbr_waf_script), XCodeEscapeSpacesForShell(root_dir), action, project_spec, target)
        self.buildPhases = []
        self.buildToolPath = sys.executable
        self.buildWorkingDirectory = ""
        self.dependencies = []
        self.name = target or action
        self.productName = target or action
        self.passBuildSettingsInEnvironment = 0

class PBXShellScriptBuildPhase(XCodeNode):
    def __init__(self):
        XCodeNode.__init__(self)
        self.buildActionMask = uintmax # default when adding a build phase manually from Xcode
        self.files = []
        self.inputPaths = []
        self.outputPaths = []
        self.runOnlyForDeploymentPostprocessing = 0
        self.shellPath = "/bin/sh"
        self.shellScript = ""

class PBXBuildPhaseBase(XCodeNode):
    def __init__(self, files):
        XCodeNode.__init__(self)
        self.buildActionMask = uintmax # default when adding a build phase manually from Xcode
        self.files = []
        for file in files:
            self.files.append(PBXBuildFile(file))
        self.runOnlyForDeploymentPostprocessing = 0

class PBXSourcesBuildPhase(PBXBuildPhaseBase):
    pass
class PBXFrameworksBuildPhase(PBXBuildPhaseBase):
    pass
class PBXResourcesBuildPhase(PBXBuildPhaseBase):
    pass

class PBXBuildFile(XCodeNode):
    def __init__(self, file_reference):
        XCodeNode.__init__(self)
        self.fileRef = file_reference._id

class PBXTargetDependency(XCodeNode):
    def __init__(self, pbx_native_target):
        XCodeNode.__init__(self)
        self.target = pbx_native_target

class PBXNativeTarget(XCodeNode):
    def __init__(self, platform_name, ctx):
        XCodeNode.__init__(self)

        self._ctx = ctx
        self._platform_name = platform_name

        self.buildPhases = []
        self.buildRules = []
        self.dependencies = []

    def init_as_exectuable(self, task_generator, settings, project_spec):
        if not task_generator:
            return

        ctx = self._ctx
        platform_name = self._platform_name

        target = task_generator.name
        node = task_generator.link_task.outputs[0]

        target_settings = copy.copy(settings)
        target_settings['PRODUCT_NAME'] = node.name

        task_gen_for_plist = task_generator

        # For the launchers they do not have the plist info files, but the game project does.
        # Search for the game project and grab its plist info file.
        launcher_name = ctx.get_default_platform_launcher_name(platform_name)
        if launcher_name and target.endswith(launcher_name):
            task_gen_for_plist = ctx.get_tgen_by_name(target[:-len(launcher_name)])

        # Not all native target are going to have a plist file (like command line executables).
        plist_files = getattr(task_gen_for_plist, 'mac_plist', None)
        if plist_files:
            plist_nodes = task_gen_for_plist.to_nodes(plist_files)
            if plist_nodes:
                target_settings['INFOPLIST_FILE'] = plist_nodes[0].abspath()
                # Since we have a plist file we are going to create an app bundle for this native target
                node = node.change_ext('.app')

        target_settings['ASSETCATALOG_COMPILER_APPICON_NAME'] = ctx.get_app_icon(target)
        target_settings['ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME'] = ctx.get_launch_image(target)
        target_settings['RUN_WAF_BUILD'] = 'YES'

        output_folder = getattr(task_generator, 'output_folder', None)
        if output_folder and isinstance(output_folder, list):
            output_folder = output_folder[0]

        output_sub_folder = getattr(task_generator, 'output_sub_folder', None)
        arch = task_generator.env['ARCH'] if task_generator.env else None

        self._generate_build_config_list(target_settings, output_folder, output_sub_folder, arch)

        self.add_prepare_for_archive_build_phase_to_target()
        self.add_waf_phase_to_target('build_{}_$CONFIGURATION  -p{} --package-projects-automatically=False'.format(platform_name, project_spec))
        self.add_waf_phase_to_target('package_{}_$CONFIGURATION  -p{} --run-xcode-for-packaging=False'.format(platform_name, project_spec))
        self.add_cleanup_after_archive_build_phase_to_target()

        self.name = target
        self.productName = task_generator.output_file_name
        self.productType = 'com.apple.product-type.application'
        self.productReference = PBXFileReference(target, node.abspath(), 'wrapper.application', 'BUILT_PRODUCTS_DIR')

    def init_as_test(self, project, target_executable, settings):
        ctx = self._ctx

        name = '{}Tests'.format(target_executable.name)

        target_product_name = target_executable.productName
        product_name = '{}Tests'.format(target_product_name)

        target_settings = copy.copy(settings)

        target_settings['PRODUCT_NAME'] = product_name
        target_settings['INFOPLIST_FILE'] = os.path.join(ctx.engine_path, XCTEST_WRAPPER_REL_PATH, 'Info.plist')

        target_settings['TEST_HOST'] = '$(BUILT_PRODUCTS_DIR)/{}.app/{}'.format(target_product_name, target_product_name)
        target_settings['BUNDLE_LOADER'] = '$(TEST_HOST)'

        target_settings['LD_RUNPATH_SEARCH_PATHS'] = [
            '$(inherited)',
            '@executable_path/Frameworks',
            '@loader_path/Frameworks'
        ]
        target_settings['COPY_PHASE_STRIP'] = 'NO'
        target_settings['CLANG_CXX_LANGUAGE_STANDARD'] = 'c++1y'

        self._generate_build_config_list(target_settings)

        test_wrapper_source = []
        for test_file in XCTEST_WRAPPER_SOURCE:
            file_ref = project.find_pbx_file_reference(os.path.join(XCTEST_WRAPPER_REL_PATH, test_file))
            if file_ref:
                test_wrapper_source.append(file_ref)

        if len(test_wrapper_source) != len(XCTEST_WRAPPER_SOURCE):
            ctx.fatal('[ERROR] Failed to find the Lumberyard XCTest Wrapper reference source file(s) in project')

        ui_kit = project.find_pbx_file_reference(os.path.join('Frameworks', 'UIKit.framework'))
        if not ui_kit:
            ctx.fatal('[ERROR] Failed to find the UIKit.framework reference in project')

        self.buildPhases = [
            PBXSourcesBuildPhase(test_wrapper_source),
            PBXFrameworksBuildPhase([ ui_kit ])
        ]

        self.dependencies = [ PBXTargetDependency(target_executable) ]

        self.name = name
        self.productName = product_name
        self.productType = 'com.apple.product-type.bundle.unit-test'

        test_target = '{}.xctest'.format(self.productName)
        test_target_path = os.path.join(target_executable.productReference.path, 'Plugins', test_target)

        self.productReference = PBXFileReference(test_target, test_target_path, sourcetree = 'BUILT_PRODUCTS_DIR', explicit_filetype = 'wrapper.cfbundle')

    def add_waf_phase_to_target(self, build_action):
        global root_dir

        cwd = XCodeEscapeSpacesForShell(root_dir)
        lmbr_waf_script = XCodeEscapeSpacesForShell(os.path.join(self._ctx.engine_path, LMBR_WAF_SCRIPT_REL_PATH))

        script = (
            'if [ \\"${RUN_WAF_BUILD}\\" = \\"YES\\" ]; then\n'
            '\t\\"\\%s\\" \\"%s\\" -cwd \\"%s\\" %s\n'
            'fi'
        ) % (sys.executable, lmbr_waf_script, cwd, build_action)

        waf_build_phase = PBXShellScriptBuildPhase()
        waf_build_phase.shellScript = script
        self.buildPhases.append(waf_build_phase)

    def add_prepare_for_archive_build_phase_to_target(self):
        '''
        When doing archive builds we need to first clean the target build directory, and create a symlink to the actual archive location.
        Xcode normally does this itself as a built-in archive step, but it fails if we've already built the project because waf will have
        created a directory in the same place that Xcode attempts to create the symlink, resulting in an archive with no executable in it!
        '''
        prepare_for_archive_build_phase = PBXShellScriptBuildPhase()
        prepare_for_archive_build_phase.runOnlyForDeploymentPostprocessing = 1 # Only run this build step for archive builds
        prepare_for_archive_build_phase.shellScript = (
            'rm -rf \\"$BUILT_PRODUCTS_DIR/$WRAPPER_NAME\\"\n'
            'ln -sf \\"$TARGET_BUILD_DIR/$WRAPPER_NAME\\" \\"$BUILT_PRODUCTS_DIR/$WRAPPER_NAME\\"'
        )
        self.buildPhases.append(prepare_for_archive_build_phase)

    def add_cleanup_after_archive_build_phase_to_target(self):
        '''
        After doing archive builds we need to delete the symlink that gets created to the target build directory,
        otherwise subsequent build or clean commands will fails when trying to create the project structure.
        '''
        cleanup_after_archive_build_phase = PBXShellScriptBuildPhase()
        cleanup_after_archive_build_phase.runOnlyForDeploymentPostprocessing = 1 # Only run this build step for archive builds
        cleanup_after_archive_build_phase.shellScript = 'rm -rf \\"$BUILT_PRODUCTS_DIR/$WRAPPER_NAME\\"'
        self.buildPhases.append(cleanup_after_archive_build_phase)

    def add_resources_build_phase_to_target(self, files):
        self.buildPhases.append(PBXResourcesBuildPhase(files))

    def add_remove_embedded_provisioning_build_phase_to_target(self):
        '''
        Remove the embedded provisioning file before each deploy to force xcode to sign the application.
        There are times when the executable is copied over but xcode does not sign it. When xcode then
        deploys the app it fails because the executable was not signed. Deleting the embeded manifest
        file forces xcode to regenerate it and sign the executable.
        '''
        remove_embedded_provisioning_build_phase = PBXShellScriptBuildPhase()
        remove_embedded_provisioning_build_phase.shellScript = 'rm -f \\"$TARGET_BUILD_DIR/$WRAPPER_NAME/embedded.mobileprovision\\"'
        self.buildPhases.append(remove_embedded_provisioning_build_phase)

    def _generate_build_config_list(self, settings, output_folder_override = None, output_sub_folder = None, archs_override = None):
        ctx = self._ctx
        platform_name = self._platform_name

        # Because of how wscript files are executed when we recurse (project_generator phase) the
        # tools that go into the lmbr_setup tools output are incomplete. For this case we call
        # get_lmbr_setup_tools_output_folder ourselves with a platform and config override to get
        # the correct output folder.
        is_lmbr_setup_tool = False
        if output_folder_override and (output_folder_override in ctx.get_lmbr_setup_tools_output_folder()):
            is_lmbr_setup_tool = True

        build_configurations = []
        for config in ctx.get_target_configurations():
            config_settings = copy.copy(settings)

            build_dir = ''

            if is_lmbr_setup_tool:
                output_folder_override = ctx.get_lmbr_setup_tools_output_folder(platform_name, config)
                build_dir = os.path.join(ctx.engine_path, output_folder_override)

            else:
                build_dir = ctx.get_output_folders(platform_name, config)[0].abspath()

            if output_sub_folder:
                build_dir = os.path.join(build_dir, output_sub_folder)

            config_settings['CONFIGURATION_BUILD_DIR'] = os.path.normpath(build_dir)

            if archs_override:
                config_settings['ARCHS'] = ' '.join(archs_override)

            build_configurations.append(XCBuildConfiguration(config, config_settings))

        self.buildConfigurationList = XCConfigurationList(build_configurations)


# Root project object
class PBXProject(XCodeNode):
    def __init__(self, name, version, ctx):
        XCodeNode.__init__(self)

        build_configuration_list = []
        for config in ctx.get_target_configurations():
            build_configuration_list.append(XCBuildConfiguration(config))
        self.buildConfigurationList = XCConfigurationList(build_configuration_list)
        self.compatibilityVersion = version[0]
        self.hasScannedForEncodings = 1;
        self.mainGroup = PBXGroup(name)
        self.projectRoot = ""
        self.projectDirPath = ""
        self.project_spec = 'all'
        self.platform_name = 'darwin_x64'
        self.config = 'debug'
        self.settings = {}
        self.targets = []
        self._objectVersion = version[1]
        self._output = PBXGroup('out')
        self.mainGroup.children.append(self._output)

    def set_project_spec(self, project_spec):
        if (project_spec):
            self.project_spec = project_spec

    def set_platform_name(self, platform):
        self.platform_name = platform

    def set_config(self, config):
        self.config = config

    def set_settings(self, settings):
        self.settings = settings

    def write(self, file):
        w = file.write
        w("// !$*UTF8*$!\n")
        w("{\n")
        w("\tarchiveVersion = 1;\n")
        w("\tclasses = {\n")
        w("\t};\n")
        w("\tobjectVersion = %d;\n" % self._objectVersion)
        w("\tobjects = {\n\n")

        XCodeNode.write(self, file)

        w("\t};\n")
        w("\trootObject = %s;\n" % self._id)
        w("}\n")

    def find_pbx_file_reference(self, file_path):
        '''
        Search the entire project source tree for a PBXFileReference specified by a relative file path.
        '''
        node = self.mainGroup
        path_parts = file_path.split('/')

        for path_part in path_parts:
            # if we've somehow managed to get a node that isn't a PBXGroup before exhausting the path,
            # the file doesn't exist
            if not isinstance(node, PBXGroup):
                return None

            for child in node.children:
                if child.name == path_part:
                    node = child
                    break
            # the file doesn't exist if all children were exhausted
            else:
                return None

        if isinstance(node, PBXFileReference):
            return node

        return None

    def add_task_gen(self, task_generator, ctx):
        target = PBXNativeTarget(self.platform_name, ctx)
        target.init_as_exectuable(task_generator, self.settings, self.project_spec)

        self.targets.append(target)
        self._output.children.append(target.productReference)

        if self.platform_name in XCTEST_WRAPPER_TARGETS:
            test_target = PBXNativeTarget(self.platform_name, ctx)
            test_target.init_as_test(self, target, self.settings)

            self.targets.append(test_target)
            self._output.children.append(test_target.productReference)

        return target


class xcode(Build.BuildContext):

    def get_settings(self):
        settings = {}
        return settings

    def get_dev_source_assets_subdir(self):
        return self.get_bootstrap_assets(self.get_target_platform_name())

    def get_release_source_assets_subdir(self):
        return '{}_paks'.format(self.get_dev_source_assets_subdir())

    def collect_source(self, task_generator):
        source_files = task_generator.to_nodes(getattr(task_generator, 'source', []))
        plist_files = task_generator.to_nodes(getattr(task_generator, 'mac_plist', []))

        source = list(set(source_files + plist_files))
        return source

    def execute(self):
        global root_dir

        self.restore()
        if not self.all_envs:
            self.load_envs()
        self.load_user_settings()
        self.recurse([self.run_dir])

        root_dir = Context.launch_dir
        xcode_project_name = self.get_xcode_project_name()
        if not xcode_project_name:
            xcode_project_name = getattr(Context.g_module, Context.APPNAME, 'project')

        project = PBXProject(xcode_project_name, ('Xcode 3.2', 46), self)
        project.set_project_spec(self.options.project_spec)

        platform_name = self.get_target_platform_name()
        project.set_platform_name(platform_name)
        project.set_settings(self.get_settings())

        resource_group = PBXGroup("Resources")
        project.mainGroup.children.append(resource_group)

        spec_modules = self.spec_modules(project.project_spec)
        project_modules = [];
        for project_name in self.get_enabled_game_project_list():
            project_modules = project_modules + self.project_and_platform_modules(project_name, platform_name)


        target_platforms = ['all']
        for platform in self.get_target_platforms():
            target_platforms.append(self.platform_to_platform_alias(platform))
            target_platforms.append(platform)

        target_platforms = set(target_platforms)

        # add the xctest wrapper source files to the project for platforms that leverage it for commandline installation
        if platform_name in XCTEST_WRAPPER_TARGETS:
            xctest_root_node = self.srcnode.make_node(XCTEST_WRAPPER_REL_PATH.split('/'))
            test_files = [ xctest_root_node.make_node(test_file) for test_file in XCTEST_WRAPPER_FILES ]

            project.mainGroup.add(self.srcnode, test_files)

            xcrun_cmd = [
                'xcrun',
                '--sdk',
                PLATFORM_SDK_NAME[platform_name],
                '--show-sdk-path'
            ]
            sdk_path = subprocess.check_output(xcrun_cmd).strip()

            # the installTest requires UIKit to show a dialog
            frameworks_group = PBXGroup('Frameworks')
            ui_kit = PBXFileReference('UIKit.framework', os.path.join(sdk_path, FRAMEWORKS_REL_PATH, 'UIKit.framework'), 'wrapper.framework', '<absolute>')
            frameworks_group.children.append(ui_kit)

            project.mainGroup.children.append(frameworks_group)


        source_files = []
        for group in self.groups:
            for task_generator in group:
                if not isinstance(task_generator, TaskGen.task_gen):
                    continue

                if (task_generator.target not in spec_modules) and (task_generator.target not in project_modules):
                    Logs.debug('xcode: Skipping %s because it is not part of the spec', task_generator.name)
                    continue

                task_generator.post()

                platforms = target_platforms.intersection(task_generator.platforms)
                if not platforms:
                    Logs.debug('xcode: Skipping %s because it is not supported on platform %s', task_generator.name, platform_name)
                    continue

                source_files = list(set(source_files + self.collect_source(task_generator)))

                # Match any C/C++ program feature
                features = Utils.to_list(getattr(task_generator, 'features', ''))
                have_feature_match = False
                for a_feature in features:
                    if re.search("c.*program", a_feature) != None:
                        have_feature_match = True
                        break
                else:
                    Logs.debug('xcode: Skipping %s because it is not a program', task_generator.name)
                    continue

                pbx_native_target = project.add_task_gen(task_generator, self)
                xcassets_path = getattr(task_generator, 'darwin_xcassets', None)
                if xcassets_path:
                    app_resources_group = PBXGroup(task_generator.name)
                    resource_group.children.append(app_resources_group)
                    xcassets_folder_node = self.engine_node.make_node(xcassets_path)
                    xcode_assets_folder_ref = PBXFileReference('xcassets', xcassets_folder_node.abspath(), 'folder.assetcatalog')
                    app_resources_group.children.append(xcode_assets_folder_ref)
                    pbx_native_target.add_resources_build_phase_to_target([xcode_assets_folder_ref])

        project.mainGroup.add(self.srcnode, source_files)
        project.targets.sort(key=lambda target: [isinstance(target, PBXLegacyTarget), target.name])

        # Create a dummy target that builds all source files so Xcode find file/symbol functionality works
        dummy_target = PBXNativeTarget(project.platform_name, self)

        source_file_references = []
        project.mainGroup.get_child_file_references_recursive(source_file_references)
        dummy_target.buildPhases = [PBXSourcesBuildPhase(source_file_references)]

        dummy_target.name = 'DummyTargetForSymbols'
        dummy_target.productName = 'DummyTargetForSymbols'
        dummy_target.productType = 'com.apple.product-type.tool'
        dummy_target.productReference = PBXFileReference('DummyTargetForSymbols', 'DummyTargetForSymbols', 'compiled.mach-o.executable', 'BUILT_PRODUCTS_DIR')

        project._output.children.append(dummy_target.productReference)
        project.targets.append(dummy_target)

        # Create game resource group/folder structure and attach it to the native
        # projects
        root_assets_folder = self.srcnode.make_node("Cache")

        for game_project in self.get_enabled_game_project_list():
            game_resources_group = PBXGroup(game_project)
            resource_group.children.append(game_resources_group)

            dev_assets_folder = root_assets_folder.make_node(game_project).make_node(self.get_dev_source_assets_subdir())
            dev_assets_folder_ref = PBXFileReference('assets_dev', dev_assets_folder.abspath(), 'folder')
            game_resources_group.children.append(dev_assets_folder_ref)

            release_assets_folder = root_assets_folder.make_node(game_project).make_node(self.get_release_source_assets_subdir())
            release_assets_folder_ref = PBXFileReference('assets_release', release_assets_folder.abspath(), 'folder')
            game_resources_group.children.append(release_assets_folder_ref)

            xcode_assets_folder = self.launch_node().make_node(self.game_code_folder(game_project) + self.get_xcode_source_assets_subdir())
            xcode_assets_folder_ref = PBXFileReference('xcassets', xcode_assets_folder.abspath(), 'folder.assetcatalog')
            game_resources_group.children.append(xcode_assets_folder_ref)

            for target in project.targets:
                launcher_name = self.get_default_platform_launcher_name(platform_name) or ''
                if isinstance(target, PBXNativeTarget) and target.name == game_project + launcher_name:
                    target.add_remove_embedded_provisioning_build_phase_to_target()
                    target.add_resources_build_phase_to_target([xcode_assets_folder_ref])

        project.mainGroup.sort_recursive()

        projectDir = self.srcnode.make_node("/%s.xcodeproj" % xcode_project_name)

        projectDir.mkdir()
        node = projectDir.make_node('project.pbxproj')
        project.write(open(node.abspath(), 'w'))


class xcode_mac(xcode):
    '''Generate an xcode project for Mac'''
    cmd = 'xcode_mac'

    def get_target_platforms(self):
        return ['darwin_x64']

    def get_target_platform_name(self):
        return 'darwin_x64'

    def get_target_configurations(self):
        return PLATFORM_MAP['darwin_x64'].get_configuration_names()

    def get_xcode_project_name(self):
        return self.get_mac_project_name()

    def get_target_assets_subdir(self):
        return 'Contents/Resources/assets'

    def get_xcode_source_assets_subdir(self):
        return '/Resources/MacLauncher/Images.xcassets'

    def get_app_icon(self, project):
        return self.get_mac_app_icon(project)

    def get_launch_image(self, project):
        return self.get_mac_launch_image(project)


class xcode_ios(xcode):
    '''Generate an xcode project for iOS'''
    cmd = 'xcode_ios'

    def get_target_platforms(self):
        return ['ios']

    def get_target_platform_name(self):
        return 'ios'

    def get_target_configurations(self):
        return PLATFORM_MAP['ios'].get_configuration_names()

    def _get_settings_file(self):
        return 'ios_settings.json'

    def get_settings(self):
        settings = xcode.get_settings(self)

        settings_root = self.root.find_node([Context.launch_dir, '_WAF_', 'apple'])

        settings_files = [
            'common_arm_settings.json',
            self._get_settings_file()
        ]

        for file_name in settings_files:

            settings_file = settings_root.find_node(file_name)
            if not settings_file:
                Logs.warn('[WARN] Could not find {} file in {}. Project will not have all necessary options set correctly'.format(file_name, settings_root.abspath()))

            else:
                settings.update(self.parse_json_file(settings_file))

        # For symroot need to have the full path so change the value from
        # relative path to a full path
        if 'SYMROOT' in settings:
            settings['SYMROOT'] = os.path.join(Context.launch_dir, settings['SYMROOT'])

        return settings

    def get_xcode_project_name(self):
        return self.get_ios_project_name()

    def get_target_assets_subdir(self):
        return 'assets'

    def get_xcode_source_assets_subdir(self):
        return '/Resources/IOSLauncher/Images.xcassets'

    def get_app_icon(self, project):
        return self.get_ios_app_icon(project)

    def get_launch_image(self, project):
        return self.get_ios_launch_image(project)


class xcode_appletv(xcode_ios):
    '''Generate an xcode project for apple tv'''
    cmd = 'xcode_appletv'

    def get_target_platforms(self):
        return ['appletv']

    def get_target_platform_name(self):
        return 'appletv'

    def get_target_configurations(self):
        return PLATFORM_MAP['appletv'].get_configuration_names()

    def _get_settings_file(self):
        return 'appletv_settings.json'

    def get_xcode_project_name(self):
        return self.get_appletv_project_name()

    def get_xcode_source_assets_subdir(self):
        return '/Resources/AppleTVLauncher/Images.xcassets'

    def get_app_icon(self, project):
        return self.get_appletv_app_icon(project)

    def get_launch_image(self, project):
        return self.get_appletv_launch_image(project)

@conf
def get_project_overrides(ctx, target):
    # Stub to satisfy context methods used for project/solution overrides.  Update if we want to
    # add similar support for xcode
    return {}


@conf
def get_file_overrides(ctx, target):
    # Stub to satisfy context methods used for project/solution overrides.  Update if we want to
    # add similar support for xcode
    return {}


@conf
def get_solution_overrides(self):
    # Stub to satisfy context methods used for project/solution overrides.  Update if we want to
    # add similar support for xcode
    return {}


@feature("parse_vcxproj")
def xcode_stub_parse_vcxproj(self):
    pass


@conf_event(after_methods=['update_valid_configurations_file'],
            after_events=['inject_generate_uber_command', 'inject_generate_module_def_command'])
def inject_xcode_commands(conf):
    """
    Make sure optional xcode project command(s) are injected into the configuration pipeline as needed
    """
    if not isinstance(conf, ConfigurationContext):
        return
    if not Utils.unversioned_sys_platform() == 'darwin':
        return

    def _add_optioned_command(option, command):
        if conf.is_option_true(option):
            # Workflow improvement: for all builds generate projects after the build
            # except when using the default build target 'utilities' then do it before
            if 'build' in Options.commands:
                build_cmd_idx = Options.commands.index('build')
                Options.commands.insert(build_cmd_idx, command)
            else:
                Options.commands.append(command)

    # Insert the optional xcode project generation commands in the configure pipeline
    _add_optioned_command('generate_ios_projects_automatically', 'xcode_ios')

    _add_optioned_command('generate_appletv_projects_automatically', 'xcode_appletv')

    _add_optioned_command('generate_mac_projects_automatically', 'xcode_mac')
