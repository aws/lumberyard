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
"""
Usage:

def options(opt):
    opt.load('android_studio')

$ waf android_studio
"""
import os, shutil

from collections import defaultdict
from pprint import pprint, pformat
from string import Template, translate

from android import SUPPORTED_APIS
from utils import write_auto_gen_header

from branch_spec import get_supported_configurations
from waf_branch_spec import BINTEMP_FOLDER

from waflib import Build, Errors, Logs, Node, TaskGen, Utils

################################################################
#                     Defaults                                 #
MIN_API_VERSION_NUMBER = sorted(SUPPORTED_APIS)[0].split('-')[1]

MIN_CMAKE_VERSION = '3.6'

ROOT_PROJECT_BUILDSCRIPT_HEADER = r'''
import org.apache.tools.ant.taskdefs.condition.Os

buildscript {
    repositories {
        google()
        jcenter()
    }
    dependencies {
        classpath "com.android.tools.build:gradle:3.3.0"
    }
}

// the WAF gradle task wrapper
class ExecWaf extends Exec {
    ExecWaf() {
        // set the working directory for the waf command
        // default to be X levels up for each of the game projects
        workingDir "${project.ext.engineRoot}"

        // base commandline tool
        if (Os.isFamily(Os.FAMILY_WINDOWS))
        {
            commandLine "cmd", "/c", "lmbr_waf.bat", "--from-android-studio"
        }
        else
        {
            commandLine "./lmbr_waf.sh", "--from-android-studio"
        }
    }
}

allprojects { project ->
    repositories {
        google()
        jcenter()
    }
    ext {
        WafTask = ExecWaf
'''

ROOT_PROJECT_BUILDSCRIPT_FOOTER = r'''
    }
    buildDir "${buildDirRoot}/java"
}
'''

TASK_GEN_HEADER = r'''
// disable the built in native tasks
tasks.whenTaskAdded { task ->
    if (task.name.contains("externalNativeBuild"))
    {
        task.setActions Arrays.asList()
    }
}

// generate all the build tasks
afterEvaluate {
    // add the waf build tasks to the build chain
    ext.platforms.each { platform ->
        ext.configurations.each { config ->
            String targetName = "${platform.capitalize()}${config.capitalize()}"
'''

TASK_GEN_WAF_SECTION = r'''
// create the WAF ${CMD} task
String waf${CMD_NAME}TaskName = "${CMD}${targetName}Waf"
String waf${CMD_NAME}Args = "${CMD}_${platform}_${config} -p all ${CMD_ARGS}"

tasks.create(name: waf${CMD_NAME}TaskName, type: WafTask, description: "lmbr_waf ${waf${CMD_NAME}Args}") {
    doFirst {
        args waf${CMD_NAME}Args.split(" ")
    }
}
if (tasks.findByPath(${INJECTION_TASK})) {
    tasks.getByPath(${INJECTION_TASK}).${INJECTION_TYPE} waf${CMD_NAME}TaskName
}
else {
    tasks.whenTaskAdded { task ->
        if (task.name.contains(${INJECTION_TASK})) {
            task.${INJECTION_TYPE} waf${CMD_NAME}TaskName
        }
    }
}
'''

TASK_GEN_APK_COPY_SECTION = r'''
// generate the copy apk task
String copyTaskName = "copy${platform.capitalize()}${config.capitalize()}Apk"
tasks.create(name: copyTaskName, type: Copy) {
    from "${buildDir}/outputs/apk/${platform}/${config}"
    into "${engineRoot}/${androidBinMap[platform][config]}"
    include "${project.name}-${platform}-${config}.apk"

    rename { String fileName ->
        fileName.replace("-${platform}-${config}", "")
    }
}
String assembleTaskName = "assemble${targetName}"
if (tasks.findByPath(assembleTaskName)) {
    tasks.getByPath(assembleTaskName).finalizedBy copyTaskName
}
else {
    tasks.whenTaskAdded { task ->
        if (task.name.contains(assembleTaskName)) {
            task.finalizedBy copyTaskName
        }
    }
}
'''

TASK_GEN_FOOTER = r'''
        }
    }
}
'''

# For more details on how to configure your build environment visit
# http://www.gradle.org/docs/current/userguide/build_environment.html
GRADLE_PROPERTIES = r'''
# Android Studio project settings overrides

# Enable Gradle as a daemon to improve the startup and execution time
org.gradle.daemon=true

# Enable parallel execution to improve execution time
org.gradle.parallel=true

# make sure configure-on-demand is disabled as it really only benefits when
# there are a large number of sub-projects
org.gradle.configureondemand=false

# bump the JVM memory limits due to the size of Lumberyard. Defaults: -Xmx1280m -XX:MaxPermSize=256m
org.gradle.jvmargs=-Xmx2048m -XX:MaxPermSize=1024m
'''

GRADLE_WRAPPER_PROPERTIES = r'''
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-4.10.1-all.zip
'''

GRADLE_WRAPPER_DIR = 'Code/Launcher/AndroidLauncher/ProjectBuilder/GradleWrapper'
#                     Defaults END                             #
################################################################


################################################################
def debug_log_value(key_word, value):
    Logs.debug('android_studio: -- %32s: %s', key_word, value)


################################################################
def defines_to_flags(defines):
    return [ '-D{}'.format(define) for define in defines if '"' not in define ]


################################################################
def to_list(value):
    if isinstance(value, set):
        return list(value)
    elif isinstance(value, list):
        return value[:]
    else:
        return [value]


################################################################
def convert_to_gradle_name(string):
    tokens = string.split('_')

    output = ""
    for index, value in enumerate(tokens):
        if index == 0:
            output = value.lower()
        else:
            output += value.title()

    return output


################################################################
def to_gradle_path(path):
    # convert to forward slash because on windows requires a double back slash
    # to be print but it only prints one in the string
    if not path:
        return ''
    return os.path.normpath(path).replace('\\', '/')


################################################################
def format_list(lst):
    return pformat(lst, indent=4).strip('[]').lstrip()

################################################################
def indent_text(text, indent, stream):
    indent_space = ' ' * indent * 4

    result = ''
    for line in text.splitlines():
        result += '{}{}\n'.format(indent_space, line)

    stream.write(result)


################################################################
class Enum(tuple):
    __getattr__ = tuple.index


################################################################
class GradleBool:
    """
    Special bool to bypass null checks and force write the value
    whereas a normal bool will be skipped if false
    """
    def __init__(self, val):
        self._val = val

    def __nonzero__(self):
        return True

    def __str__(self):
        return str(self._val).lower()


################################################################
class GradleList(list):
    """
    Wrapped list to determine when to use special formating
    """
    def __init__(self, *k):
        self.extend(k)


################################################################
class GradleNode(object):

    def __nonzero__(self):
        return any(self.__dict__.values())

    def to_string(self, obj):
        if isinstance(obj, bool):
            return str(obj).lower()

        elif isinstance(obj, str):
            return '"{}"'.format(obj)

        elif isinstance(obj, GradleList):
            return format_list(obj).replace("'", '"')

        else:
            return str(obj)

    def write_value(self, obj, stream, indent = 0):
        next_indent = indent + 1

        if isinstance(obj, GradleNode):
            obj.write(stream, indent)

        elif isinstance(obj, dict):
            for key, value in obj.iteritems():
                indent_text('{} {}'.format(convert_to_gradle_name(key), self.to_string(value)), next_indent, stream)

    def write_internal_dict(self, stream, indent = 0):
        next_indent = indent + (0 if getattr(self, 'is_root_node', False) else 1)
        private_attrib_prefix = '_'

        for attribute, value in self.__dict__.iteritems():

            if not value or attribute.startswith(private_attrib_prefix):
                continue

            elif isinstance(value, GradleNode):
                value.write(stream, next_indent)

            elif isinstance(value, dict):
                for key, subvalue in value.iteritems():
                    if not subvalue:
                        continue

                    if hasattr(subvalue, 'gradle_name'):
                        self.write_value(subvalue, stream, next_indent)
                    else:
                        indent_text('{} {{'.format(key), next_indent, stream)
                        self.write_value(subvalue, stream, next_indent)
                        indent_text('}', next_indent, stream)

            elif isinstance(value, GradleList):
                indent_text('{} {}'.format(convert_to_gradle_name(attribute), self.to_string(value)), next_indent, stream)

            elif isinstance(value, list):
                for elem in value:
                    indent_text('{} {}'.format(convert_to_gradle_name(attribute), self.to_string(elem)), next_indent, stream)

            else:
                indent_text('{} {}'.format(convert_to_gradle_name(attribute), self.to_string(value)), next_indent, stream)

    def write(self, stream, indent = 0):

        if hasattr(self.__class__, 'gradle_name'):
            indent_text('{} {{'.format(self.__class__.gradle_name), indent, stream)
            self.write_internal_dict(stream, indent)
            indent_text('}', indent, stream)

        else:
            self.write_internal_dict(stream, indent)


################################################################
class SigningConfigRef:
    """
    Wrapper class to provide custom formating
    """

    def __init__(self, config_name = ''):
        self.config_name = config_name

    def __nonzero__(self):
        return True if self.config_name else False

    def __str__(self):
        return 'signingConfigs.{}'.format(self.config_name)


################################################################
class SigningConfigs(GradleNode):
    gradle_name = 'signingConfigs'

    class FileRef:

        def __init__(self, path = ''):
            self.path = path

        def __str__(self):
            return 'file("{}")'.format(to_gradle_path(self.path))

    class Config(GradleNode):

        def __init__(self):
            self.key_alias = ''
            self.key_password = ''
            self.store_file = SigningConfigs.FileRef()
            self.store_password = ''

        def __nonzero__(self):
            return all(self.__dict__.values())

        def __setattr__(self, name, value):
            if name == 'store_file':
                value = SigningConfigs.FileRef(value)
            self.__dict__[name] = value

    def __init__(self):
        self.configs = defaultdict(SigningConfigs.Config)

    def __nonzero__(self):
        return any(self.configs.values())

    def add_signing_config(self, config_name, **config_props):
        config = SigningConfigs.Config()

        for attribute in config.__dict__.keys():
            setattr(config, attribute, config_props.get(attribute, ''))

        self.configs[config_name] = config


################################################################
class CMakeLists:

    LibType = Enum(['Static', 'Shared'])

    def __init__(self):
        self.lib_type = CMakeLists.LibType.Shared
        self.source = []
        self.includes = []
        self.exports = []
        self.dependencies = []

    def __nonzero__(self):
        return any(self.source)

    def write(self, module_name, stream):
        # strips out any quotes and commas from the stringified list
        def format_cmake_list(lst):
            return translate(format_list(lst), None, '\',')

        library_def = [
            module_name,
            CMakeLists.LibType[self.lib_type].upper()
        ] + self.source
        stream.write('add_library({})\n'.format(format_cmake_list(library_def)))

        includes = [ module_name ]

        if self.exports:
            includes.extend(['PUBLIC'] + self.exports)

        if self.includes:
            includes.extend(['PRIVATE'] + self.includes)

        if len(includes) >= 2:
            stream.write('target_include_directories({})\n'.format(format_cmake_list(includes)))

        if self.dependencies:
            deps = [module_name] + self.dependencies
            stream.write('target_link_libraries({})\n'.format(format_cmake_list(deps)))


################################################################
class NativeBuildPaths(GradleNode):
    gradle_name = 'externalNativeBuild'

    class Cmake(GradleNode):
        gradle_name = 'cmake'

        def __init__(self):
            self.path = ''
            self.build_staging_directory = ''

    def __init__(self):
        self.cmake = NativeBuildPaths.Cmake()

    def set_default_paths(self):
        self.cmake.path = '${rootDir}/CMakeLists.txt'
        self.cmake.build_staging_directory = '${buildDirRoot}'


################################################################
class NativeBuildFlags(GradleNode):
    gradle_name = 'externalNativeBuild'

    class Cmake(GradleNode):
        gradle_name = 'cmake'

        def __init__(self):
            self.targets = GradleList()
            self.arguments = GradleList()
            self.abi_filters = GradleList()
            self.c_flags = GradleList()
            self.cpp_flags = GradleList()

    def __init__(self):
        self.cmake = NativeBuildFlags.Cmake()

    def add_targets(self, targets):
        self.cmake.targets.extend(to_list(targets))

    def add_abi_filters(self, abi_filters):
        self.cmake.abi_filters.extend(to_list(abi_filters))

    def add_general_compiler_flags(self, flags):
        flags_list = to_list(flags)

        self.cmake.c_flags.extend(flags_list)
        self.cmake.cpp_flags.extend(flags_list)


################################################################
class DefaultConfig(GradleNode):
    gradle_name = 'defaultConfig'

    def __init__(self):
        self.application_id = ''
        self.min_sdk_version = 0
        self.target_sdk_version = 0
        self.signing_config = SigningConfigRef()
        self.ndk = NativeBuildFlags()

    def set_properties(self, **props):
        self.application_id = props.get('application_id', '')
        self.min_sdk_version = props.get('min_sdk', MIN_API_VERSION_NUMBER)
        self.target_sdk_version = props.get('target_sdk', MIN_API_VERSION_NUMBER)
        self.signing_config = SigningConfigRef(props.get('signing_config_ref', ''))

    def set_ndk_properties(self, **props):
        targets = props.get('targets', [])
        self.ndk.add_targets(targets)

        flags = props.get('ndk_flags', [])
        self.ndk.add_general_compiler_flags(flags)


################################################################
class Sources(GradleNode):
    gradle_name = 'sourceSets'

    class Paths(GradleNode):

        def __init__(self):
            self.src_dirs = GradleList()
            self.src_file = ''

        def add_src_paths(self, src_paths):
            for path in to_list(src_paths):
                self.src_dirs.append(to_gradle_path(path))

        def set_src_file(self, file_path):
            self.src_file = to_gradle_path(file_path)

    class Set(GradleNode):

        def __init__(self):
            self.paths = defaultdict(Sources.Paths)

        def __nonzero__(self):
            return any(self.paths.values())

        def set_java_properties(self, **props):
            java_src = props.get('java_src', [])
            self.paths['java'].add_src_paths(java_src)

            aidl_src = props.get('aidl_src', [])
            self.paths['aidl'].add_src_paths(aidl_src)

            res_src = props.get('res_src', [])
            self.paths['res'].add_src_paths(res_src)

            manifest_path = props.get('manifest_path', '')
            self.paths['manifest'].set_src_file(manifest_path)


    def __init__(self):
        self.variants = defaultdict(Sources.Set)

    def __nonzero__(self):
        return any(self.variants.values())

    def set_main_properties(self, **props):
        self.set_variant_properties('main', **props)

    def set_variant_properties(self, variant_name, **props):
        variant = self.variants[variant_name]
        variant.set_java_properties(**props)


################################################################
class Builds(GradleNode):
    gradle_name = 'buildTypes'

    class Type(GradleNode):

        def __init__(self):
            self.ndk = NativeBuildFlags()
            self.debuggable = GradleBool(False)
            self.jni_debuggable = GradleBool(False)

        def set_debuggable(self, debuggable):
            self.debuggable = GradleBool(debuggable)
            self.jni_debuggable = GradleBool(debuggable)

        def add_ndk_compiler_flags(self, flags):
            self.ndk.add_general_compiler_flags(flags)

    def __init__(self):
        self.types = defaultdict(Builds.Type)

    def add_build_type(self, build_name, **build_props):
        debuggable = build_name in ('debug', 'profile')

        build_type = self.types[build_name]
        build_type.set_debuggable(build_props.get('debuggable', debuggable))

        ndk_flags = build_props.get('ndk_flags', [])
        build_type.add_ndk_compiler_flags(to_list(ndk_flags))


################################################################
class Products(GradleNode):
    gradle_name = 'productFlavors'

    class Flavor(GradleNode):

        def __init__(self):
            self.ndk = NativeBuildFlags()
            self.dimension = GradleList('arch')

        def add_abi_filters(self, abis):
            self.ndk.add_abi_filters(abis)

        def add_ndk_compiler_flags(self, flags):
            self.ndk.add_general_compiler_flags(flags)

    def __init__(self):
        self.flavors = defaultdict(Products.Flavor)

    def add_product_flavor(self, flavor_name, **flavor_props):
        product_flavor = self.flavors[flavor_name]

        abis = flavor_props.get('abis', [])
        product_flavor.add_abi_filters(abis)

        flags = flavor_props.get('ndk_flags', [])
        product_flavor.add_ndk_compiler_flags(flags)


################################################################
class JavaCompileOptions(GradleNode):
    gradle_name = 'compileOptions'

    class Version:

        def __init__(self, version):
            self._version = version.replace('.', '_')

        def __nonzero__(self):
            return True

        def __str__(self):
            return 'JavaVersion.VERSION_{}'.format(self._version)

    def __init__(self, java_version):
        self.source_compatibility = JavaCompileOptions.Version(java_version)
        self.target_compatibility = JavaCompileOptions.Version(java_version)


################################################################
class LintOptions(GradleNode):
    gradle_name = 'lintOptions'

    def __init__(self):
        self.check_release_builds = GradleBool(False)


################################################################
class Android(GradleNode):
    gradle_name = 'android'

    def __init__(self):
        self.compile_sdk_version = MIN_API_VERSION_NUMBER
        self.build_tools_version = ''
        self.default_config = DefaultConfig()
        self.ndk = NativeBuildPaths()
        self.build_types = Builds()
        self.flavor_dimensions = GradleList('arch')
        self.product_flavors = Products()
        self.sources = Sources()
        self.signing_configs = SigningConfigs()

    def set_general_properties(self, **props):
        if 'sdk_version' in props:
            self.compile_sdk_version = props['sdk_version']

        if 'build_tools_version' in props:
            self.build_tools_version = props['build_tools_version']

        self.default_config.set_properties(**props)

    def set_default_ndk_properties(self, **props):
        self.ndk.set_default_paths()
        self.default_config.set_ndk_properties(**props)

    def add_build_type(self, build_name, **props):
        self.build_types.add_build_type(build_name, **props)

    def add_product_flavor(self, flavor_name, **props):
        self.product_flavors.add_product_flavor(flavor_name, **props)

    def set_main_source_paths(self, **paths):
        self.sources.set_main_properties(**paths)

    def add_java_compile_options(self, java_version):
        self.java_options = JavaCompileOptions(java_version)
        self.lint_options = LintOptions()


################################################################
class Module(GradleNode):
    is_root_node = True

    Type = Enum([ 'Application', 'Library', 'Native' ])

    class Dependency:

        Linkage = Enum([ 'API', 'Implementation', 'Compile_Only', 'Runtime_Only' ])
        Type = Enum([ 'Project', 'Library', 'Files' ])

        def __init__(self, name, dependency_type, linkage):
            self.name = name
            self._linkage = linkage
            self._type = dependency_type

        @property
        def linkage(self):
            return Module.Dependency.Linkage[self._linkage].lower()

        def __str__(self):
            format_strings = {
                Module.Dependency.Type.Project : 'project(":{}")',
                Module.Dependency.Type.Library : '"{}"',
                Module.Dependency.Type.Files : 'files("{}")',
            }
            return format_strings[self._type].format(self.name)

    class Dependencies(GradleNode):
        gradle_name = 'dependencies'

        def __getattr__(self, name):
            if not hasattr(self, name):
                setattr(self, name, [])
            return self.__dict__[name]

        def _add_dependency(self, dependency):
            deps = getattr(self, dependency.linkage)
            deps.append(dependency)

        def add_project(self, project, linkage = None):
            linkage = linkage or Module.Dependency.Linkage.API
            dependency = Module.Dependency(project, Module.Dependency.Type.Project, linkage)
            self._add_dependency(dependency)

        def add_library(self, library, linkage = None):
            linkage = linkage or Module.Dependency.Linkage.API
            dependency = Module.Dependency(library, Module.Dependency.Type.Library, linkage)
            self._add_dependency(dependency)

        def add_file(self, file, linkage = None):
            linkage = linkage or Module.Dependency.Linkage.API
            dependency = Module.Dependency(file, Module.Dependency.Type.Files, linkage)
            self._add_dependency(dependency)

    def __init__(self, name, module_type):
        self.android = Android()
        self.dependencies = Module.Dependencies()
        self._name = name
        self._type = module_type
        self._cmake = CMakeLists()

    @property
    def name(self):
        return self._name

    @property
    def type(self):
        return self._type

    def write_gradle_file(self, module_dir):
        # native only modules no longer get gradle.build files
        if self.type == Module.Type.Native:
            return False

        build_gradle_node = module_dir.make_node('build.gradle')

        with open(build_gradle_node.abspath(), 'w') as build_file:
            write_auto_gen_header(build_file)

            plugin_type = Module.Type[self.type].lower()
            build_file.write('apply plugin: "com.android.{}"\n\n'.format(plugin_type))

            self.write(build_file)

            if self.type == Module.Type.Application:
                build_file.write(TASK_GEN_HEADER)

                def write(text):
                    indent_text(text, 3, build_file)

                waf_section_tmpl = Template(TASK_GEN_WAF_SECTION)
                waf_build_args = {
                    'CMD' : 'build',
                    'CMD_NAME' : 'Build',
                    'INJECTION_TYPE' : 'finalizedBy',
                    'INJECTION_TASK' : '"externalNativeBuild${targetName}"'
                }

                enable_game_arg = '--enabled-game-projects={}'.format(self.name.replace('Launcher', ''))

                waf_build_args['CMD_ARGS'] = '{} --deploy-android=False'.format(enable_game_arg)
                write(waf_section_tmpl.safe_substitute(waf_build_args))

                write(TASK_GEN_APK_COPY_SECTION)

                write(waf_section_tmpl.safe_substitute({
                    'CMD' : 'deploy',
                    'CMD_NAME' : 'Deploy',
                    'CMD_ARGS' : '{} --deploy-android=True'.format(enable_game_arg),
                    'INJECTION_TYPE' : 'finalizedBy',
                    'INJECTION_TASK' : 'copyTaskName'
                }))

                build_file.write(TASK_GEN_FOOTER)
        return True

    def write_cmake_lists_file(self, module_dir):
        if not self._cmake:
            return False

        cmake_lists_node = module_dir.make_node('CMakeLists.txt')

        with open(cmake_lists_node.abspath(), 'w') as cmake_file:
            write_auto_gen_header(cmake_file)
            self._cmake.write(self.name, cmake_file)
        return True

    def apply_general_settings(self, ctx, java_version):
        if self.type == Module.Type.Native:
            return

        self.android.add_java_compile_options(java_version)

        if self.type == Module.Type.Application:
            signing_props = {
                'key_alias' : ctx.get_android_dev_keystore_alias(),
                'store_file' : ctx.get_android_dev_keystore_path(),
                'key_password' : ctx.options.dev_key_pass,
                'store_password' : ctx.options.dev_store_pass
            }
            self.android.signing_configs.add_signing_config('Development', **signing_props)

    def apply_build_types(self, build_types):
        for type_name, type_props in build_types.iteritems():
            module_props = {}

            if self.type != Module.Type.Library:
                module_props['ndk_flags'] = defines_to_flags(type_props.get('defines', []))

            self.android.add_build_type(type_name, **module_props)

    def apply_product_flavors(self, product_flavors):
        for flavor_name, flavor_props in product_flavors.iteritems():
            module_props = {}

            if self.type != Module.Type.Library:
                module_props['abis'] = flavor_props['abi']
                module_props['ndk_flags'] = defines_to_flags(flavor_props.get('defines', []))

            self.android.add_product_flavor(flavor_name, **module_props)

    def process_target(self, proj_props, target_name, task_generator):
        android = self.android
        module_type = self.type
        ctx = proj_props.ctx

        target_sdk = getattr(task_generator, 'sdk_version', ctx.env['ANDROID_SDK_VERSION_NUMBER'])
        default_properties = {
            'sdk_version' : target_sdk,
            'build_tools_version' : ctx.get_android_build_tools_version(),
            'min_sdk' : getattr(task_generator, 'min_sdk', ctx.env['ANDROID_NDK_PLATFORM_NUMBER']),
            'target_sdk' : target_sdk
        }

        if module_type == Module.Type.Application:
            game_project = getattr(task_generator, 'project_name', '')
            default_properties['application_id'] = ctx.get_android_package_name(game_project)
            default_properties['signing_config_ref'] = 'Development'

        android.set_general_properties(**default_properties)

        if module_type in (Module.Type.Application, Module.Type.Library):

            java_src_paths = ctx.collect_task_gen_attrib(task_generator, 'android_java_src_path')
            aidl_src_paths = ctx.collect_task_gen_attrib(task_generator, 'android_aidl_src_path')
            manifest_path = ctx.collect_task_gen_attrib(task_generator, 'android_manifest_path')
            resource_src_paths = ctx.collect_task_gen_attrib(task_generator, 'android_res_path')

            if module_type == Module.Type.Application:
                targets = [ target_name ]

                game_project = getattr(task_generator, 'project_name', None)
                for tsk_gen in ctx.project_tasks:
                    module_name = tsk_gen.target

                    # skip the launchers / same module, those source paths were already added above
                    if module_name.endswith('AndroidLauncher'):
                        continue

                    if ctx.is_module_for_game_project(module_name, game_project, None):
                        java_src_paths += ctx.collect_task_gen_attrib(tsk_gen, 'android_java_src_path')
                        aidl_src_paths += ctx.collect_task_gen_attrib(tsk_gen, 'android_aidl_src_path')

                        targets.append(module_name)

                # process the task specific defines
                common_defines = ctx.collect_task_gen_attrib(task_generator, 'defines')
                debug_log_value('Common Defines', common_defines)

                flags = defines_to_flags(proj_props.defines + common_defines)
                android.set_default_ndk_properties(
                    targets = sorted(targets),
                    ndk_flags = flags
                )

                for config in proj_props.build_types:
                    config_defines = ctx.collect_task_gen_attrib(task_generator, '{}_defines'.format(config))
                    debug_log_value('{} Defines'.format(config.title()), config_defines)

                    if config_defines:
                        config_flags = defines_to_flags(config_defines)
                        android.add_build_type(config, ndk_flags = config_flags)

                for target in proj_props.product_flavors:
                    target_defines = ctx.collect_task_gen_attrib(task_generator, '{}_defines'.format(target))
                    debug_log_value('{} Defines'.format(target.title()), target_defines)

                    if target_defines:
                        target_flags = defines_to_flags(target_defines)
                        android.add_product_flavor(target, ndk_flags = target_flags)

            debug_log_value('Source Paths (java)', java_src_paths)
            debug_log_value('Source Paths (aidl)', aidl_src_paths)

            android.set_main_source_paths(
                java_src = java_src_paths,
                aidl_src = aidl_src_paths,
                res_src = resource_src_paths,
                manifest_path = (manifest_path[0] if manifest_path else '')
            )

        # process info required to make the cmake lists file
        task_type = getattr(task_generator, '_type', None)
        if task_type in ('stlib', 'shlib'):
            cmake_info = self._cmake

            if task_type == 'stlib':
                cmake_info.lib_type = CMakeLists.LibType.Static
            else:
                cmake_info.lib_type = CMakeLists.LibType.Shared

            # Android Studio will attempt to extrapolate a common root JNI source path from all native
            # module sources listed in the project's make files by default.  This is important because
            # that path is used for file indexing and IDE symbol generation.  Unfortunately Lumberyard
            # has a few modules with mixed location source (specifically from 3rd Party) and causes the
            # search path to encompass so much that Android Studio will easily run out of memory building
            # symbols.  This can even happen when including the uber files from BinTemp in the module
            # definition.  So those files need to be removed from the make file module definition, which
            # is fine as they aren't used in the actual build.
            jni_src_nodes = ctx.collect_task_gen_attrib(task_generator, 'source')
            base_paths = set([
                to_gradle_path(ctx.path.abspath()),
                to_gradle_path(ctx.root.make_node(ctx.engine_path).abspath()),
            ])
            jni_src_paths = []
            for src_node in jni_src_nodes:
                src_path = to_gradle_path(src_node.abspath())

                if not any(base_path for base_path in base_paths if src_path.startswith(base_path)):
                    continue
                if BINTEMP_FOLDER in src_path:
                    continue

                jni_src_paths.append(src_path)
            cmake_info.source = sorted(jni_src_paths)

            def fix_includes_paths(paths):
                fixed_paths = []
                for path in paths:
                    corrected_path = None
                    if isinstance(path, str):
                        if os.path.isabs(path):
                            corrected_path = path
                        elif path == '.':
                            corrected_path = task_generator.path.abspath()
                        else:
                            corrected_path = task_generator.path.make_node(path).abspath()
                    else:
                        corrected_path = path.abspath()
                    fixed_paths.append(to_gradle_path(corrected_path))
                return fixed_paths

            includes = ctx.collect_task_gen_attrib(task_generator, 'includes')
            cmake_info.includes = fix_includes_paths(includes)

            exports = ctx.collect_task_gen_attrib(task_generator, 'export_includes')
            cmake_info.exports = fix_includes_paths(exports)

            # for some reason get_module_uses doesn't work correctly on the launcher
            # tasks, so we need to manually create the use dependency tree
            all_native_uses = []
            if module_type == Module.Type.Application:
                local_uses = ctx.collect_task_gen_attrib(task_generator, 'use')
                all_native_uses = local_uses[:]
                for use in local_uses:
                    all_native_uses.extend(ctx.get_module_uses(use, proj_props.project_spec))

            elif module_type == Module.Type.Native:
                all_native_uses = ctx.get_module_uses(target_name, proj_props.project_spec)

            project_names = [ tgen.target for tgen in ctx.project_tasks ]
            cmake_info.dependencies = list(set(project_names).intersection(all_native_uses))
            debug_log_value('Uses (native)', list(cmake_info.dependencies))

    def process_module_dependencies(self, project, task_generator):
        deps = self.dependencies

        modules = project.ctx.collect_task_gen_attrib(task_generator, 'module_dependencies')
        for module_dep in modules:
            deps.add_project(module_dep)

        files = project.ctx.collect_task_gen_attrib(task_generator, 'file_dependencies')
        for file_dep in files:
            deps.add_file(file_dep)

        # Look for the android specific uses
        if self.type == Module.Type.Application:
            def _get_task_gen(task_name):
                try:
                    return project.ctx.get_tgen_by_name(task_name)
                except:
                    return None

            game_project = getattr(task_generator, 'project_name', '')
            apk_task_name = '{}_APK'.format(game_project)

            apk_task = _get_task_gen(apk_task_name)
            if apk_task:
                uses_added = []

                uses = project.ctx.collect_task_gen_attrib(apk_task, 'use')

                for tsk_gen in project.ctx.project_tasks:
                    module_name = tsk_gen.target

                    # skip the launchers / same module, those source paths were already added above
                    if module_name.endswith('AndroidLauncher'):
                        continue

                    if project.ctx.is_module_for_game_project(module_name, game_project, None):
                        module_uses = project.ctx.collect_task_gen_attrib(tsk_gen, 'use')
                        uses = uses + module_uses

                uses = list(set(uses))

                for use in uses:
                    use_task_gen = _get_task_gen(use)
                    if use_task_gen:
                        use_task_gen.post()

                        if not hasattr(use_task_gen, 'aar_task'):
                            continue

                        android_studio_name = getattr(use_task_gen, 'android_studio_name', None)
                        if android_studio_name:
                            if android_studio_name.startswith('file:'):
                                android_studio_name = android_studio_name[5:]
                                deps.add_file(android_studio_name)
                            else:
                                deps.add_library(android_studio_name)
                            uses_added.append(android_studio_name)
                        else:
                            deps.add_project(use)
                            uses_added.append(use)

                debug_log_value('Uses (android)', uses_added)


################################################################
class AndroidStudioProject:

    def __init__(self, ctx):
        self.ctx = ctx
        self.project_spec = ''
        self.java_version = ''
        self.defines = []
        self.build_types = defaultdict(dict)
        self.product_flavors = defaultdict(dict)
        self.projects = {}

    def set_common_properties(self, java_version, defines):
        self.java_version = java_version
        self.defines = to_list(defines)

    def set_build_types(self, build_types, type_defines):
        for type_name in build_types:
            type_props = self.build_types[type_name]
            type_props['defines'] = type_defines.get(type_name, [])

    def set_product_flavors(self, product_flavors, flavor_defines, flavor_abis):
        for flavor_name in product_flavors:
            flavor_props = self.product_flavors[flavor_name]
            flavor_props['abi'] = flavor_abis[flavor_name]
            flavor_props['defines'] = flavor_defines.get(flavor_name, [])

    def add_target_to_project(self, project_name, module_type, project_task_gen):
        Logs.debug('android_studio: Added Module - {} - to project'.format(project_name))

        android_module = Module(project_name, module_type)

        android_module.apply_general_settings(self.ctx, self.java_version)
        android_module.apply_build_types(self.build_types)
        android_module.apply_product_flavors(self.product_flavors)

        android_module.process_target(self, project_name, project_task_gen)

        android_module.process_module_dependencies(self, project_task_gen)

        self.projects[project_name] = android_module

    def write_project(self, root_node):
        try:
            def open_file(name):
                file_node = root_node.make_node(name)
                return open(file_node.abspath(), 'w')

            gradle_settings_file = open_file('settings.gradle')
            write_auto_gen_header(gradle_settings_file)

            cmake_lists_file = open_file('CMakeLists.txt')
            write_auto_gen_header(cmake_lists_file)
            cmake_lists_file.write('cmake_minimum_required(VERSION {})\n'.format(MIN_CMAKE_VERSION))

            for target_name in sorted(self.projects.keys()):
                project = self.projects[target_name]

                module_dir = root_node.make_node(target_name)
                module_dir.mkdir()

                if project.write_gradle_file(module_dir):
                    gradle_settings_file.write('include ":{}"\n'.format(target_name))

                if project.write_cmake_lists_file(module_dir):
                    cmake_lists_file.write('add_subdirectory({})\n'.format(target_name))

            gradle_settings_file.close()
            cmake_lists_file.close()
        except Exception as err:
            self.ctx.fatal('[ERROR] Failed to write out Android Studio project files.  Reason: {}'.format(err))


################################################################
def options(opt):
    group = opt.add_option_group('android-studio config')

    # disables the apk packaging process so android studio can do it
    group.add_option('--from-android-studio', dest = 'from_android_studio', action = 'store_true', default = False, help = 'INTERNAL USE ONLY for Experimental Andorid Studio support')


################################################################
class android_studio(Build.BuildContext):
    cmd = 'android_studio'

    def get_target_platforms(self):
        """
        Used in cryengine_modules get_platform_list during project generation
        """
        android_targets = [ target for target in self.get_supported_platforms() if 'android' in target ]
        return [ 'android' ] + android_targets

    def collect_task_gen_attrib(self, task_generator, attribute, *modifiers):
        """
        Helper for getting an attribute from the task gen with optional modifiers
        """
        result = to_list(getattr(task_generator, attribute, []))
        for mod in modifiers:
            mod_attrib = '{}_{}'.format(mod, attribute)
            result.extend(to_list(getattr(task_generator, mod_attrib, [])))
        return result

    def execute(self):
        ''' Entry point of the project generation '''

        # restore the environments
        self.restore()
        if not self.all_envs:
            self.load_envs()

        # validate the base project directory exists
        android_root = self.path.make_node(self.get_android_project_relative_path())
        if not os.path.exists(os.path.join(android_root.abspath(), 'wscript')):
            raise Errors.WafError('[ERROR] Base android projects not generated.  Re-run the configure command.')

        self.load_user_settings()
        Logs.info("[WAF] Executing 'android_studio' in '{}'".format(self.variant_dir))
        self.recurse([ self.run_dir ])

        # check the apk signing environment
        if self.get_android_build_environment() == 'Distribution':
            Logs.warn('[WARN] The Distribution build environment is not currently supported in Android Studio, falling back to the Development build environment.')

        # get the core build settings
        android_platforms = self.get_platform_alias('android')

        android_config_sets = []
        for platform in android_platforms:
            android_config_sets.append(set(get_supported_configurations(platform)))
        android_configs = list(set.intersection(*android_config_sets))

        all_defines = []
        platform_defines = defaultdict(list)
        config_defines = defaultdict(list)

        platform_abis = defaultdict(set)
        java_versions = set()

        for platform in android_platforms:
            current_platform_defines = platform_defines[platform]
            current_platform_abis = platform_abis[platform]

            for config in android_configs:
                variant = '{}_{}'.format(platform, config)
                env = self.all_envs[variant]

                variant_defines = set(env['DEFINES'])

                all_defines.append(variant_defines)
                current_platform_defines.append(variant_defines)
                config_defines[config].append(variant_defines)

                current_platform_abis.add(env['ANDROID_ARCH'])
                java_versions.add(env['JAVA_VERSION'])

            if len(platform_abis[platform]) != 1:
                self.fatal('[ERROR] Multiple or no architectures specified across configurations for Android target - {}.  '
                            'Only one type can be specified in ANDROID_ARCH.'.format(platform))

        if len(java_versions) != 1:
            self.fatal('[ERROR] Multiple or no Java compatibility versions detected across Android build targets.  '
                        'Only one version can be specified in JAVA_VERSION')

        def filter_unique(source_map, common_set):
            return { key : set.intersection(*value).difference(common_set) for key, value in source_map.iteritems() }

        common_defines = set.intersection(*all_defines)
        unique_platform_defines = filter_unique(platform_defines, common_defines)
        unique_config_defines = filter_unique(config_defines, common_defines)

        # collect all the possible modules
        acceptable_platforms = ['all', 'android'] + android_platforms

        project_spec = self.options.project_spec or 'all'
        modules = self.spec_modules(project_spec)[:]
        modules.append('native_activity_glue') # hack :(

        for project_name in self.get_enabled_game_project_list():
            modules.extend(self.project_and_platform_modules(project_name, acceptable_platforms))

        # validate all the possible spec modules and find their respective task generators
        self.project_tasks = []
        for group in self.groups:
            for task_generator in group:
                if not isinstance(task_generator, TaskGen.task_gen):
                    continue

                target_name = task_generator.target or task_generator.name
                task_platforms = self.get_module_platforms(target_name)

                is_in_spec = (target_name in modules)
                is_android_enabled = any(set.intersection(task_platforms, acceptable_platforms))

                game_project = getattr(task_generator, 'project_name', target_name)
                is_game_project = (game_project in self.get_enabled_game_project_list())
                if is_android_enabled and is_game_project:
                    is_android_enabled = self.get_android_settings(game_project)

                if not (is_in_spec and is_android_enabled):
                    Logs.debug('android_studio: Skipped module - %s - is not in the spec or not configure for Android', target_name)
                    continue

                self.project_tasks.append(task_generator)

        debug_log_value('project_tasks', [tsk.name for tsk in self.project_tasks])

        # get the core build settings
        project = AndroidStudioProject(self)
        project.project_spec = project_spec

        project.set_common_properties(list(java_versions)[0], common_defines)
        project.set_build_types(android_configs, unique_config_defines)
        project.set_product_flavors(android_platforms, unique_platform_defines, platform_abis)

        # process the modules to be added to the project
        for task_generator in self.project_tasks:
            target_name = task_generator.target

            game_project = getattr(task_generator, 'project_name', target_name)
            module_type = Module.Type.Native

            if game_project in self.get_enabled_game_project_list() and game_project is not target_name:
                target_name = self.get_executable_name(game_project)
                module_type = Module.Type.Application

            task_generator.post()
            self.process_module(project, target_name, android_root, module_type, task_generator)

        project.write_project(android_root)

        # generate the root gradle build script
        root_build = android_root.make_node('build.gradle')
        try:
            with open(root_build.abspath(), 'w') as root_build_file:
                write_auto_gen_header(root_build_file)
                root_build_file.write(ROOT_PROJECT_BUILDSCRIPT_HEADER)

                def write(text, indent = 2):
                    indent_text(text, indent, root_build_file)

                relpath_to_root = to_gradle_path(self.path.path_from(android_root))
                write('engineRoot = "${{rootDir}}/{}/"'.format(relpath_to_root))
                write('buildDirRoot = "${engineRoot}/BinTemp/android_studio/${project.name}"')

                platforms_string = pformat(android_platforms).replace("'", '"')
                write('platforms = {}'.format(platforms_string))

                configs_string = pformat(android_configs).replace("'", '"')
                write('configurations = {}'.format(configs_string))

                write('androidBinMap = [')
                for platform in android_platforms:
                    write('"{}" : ['.format(platform), 3)
                    for config in android_configs:
                        write('"{}" : "{}",'.format(config, self.get_output_folders(platform, config)[0]), 4)
                    write('],', 3)
                write(']')

                root_build_file.write(ROOT_PROJECT_BUILDSCRIPT_FOOTER)
        except Exception as err:
            self.fatal(str(err))

        # generate the android local properties file
        local_props = android_root.make_node('local.properties')
        try:
            with open(local_props.abspath(), 'w') as props_file:
                write_auto_gen_header(props_file)

                sdk_path = os.path.normpath(self.get_env_file_var('LY_ANDROID_SDK'))
                ndk_path = os.path.normpath(self.get_env_file_var('LY_ANDROID_NDK'))

                # windows is really picky about it's file paths
                if Utils.unversioned_sys_platform() == "win32":
                    sdk_path = sdk_path.replace('\\', '\\\\').replace(':', '\\:')
                    ndk_path = ndk_path.replace('\\', '\\\\').replace(':', '\\:')

                props_file.write('sdk.dir={}\n'.format(sdk_path))
                props_file.write('ndk.dir={}\n'.format(ndk_path))
        except Exception as err:
            self.fatal(str(err))

        # generate the gradle properties file
        gradle_props = android_root.make_node('gradle.properties')
        try:
            with open(gradle_props.abspath(), 'w') as props_file:
                write_auto_gen_header(props_file)
                props_file.write(GRADLE_PROPERTIES)
        except Exception as err:
            self.fatal(str(err))

        # generate the gradle wrapper
        if self.is_engine_local():
            source_node = self.path.make_node(GRADLE_WRAPPER_DIR)
        else:
            source_node = self.root.make_node(os.path.abspath(os.path.join(self.engine_path, GRADLE_WRAPPER_DIR)))

        wrapper_root_node = android_root.make_node([ 'gradle', 'wrapper' ])
        wrapper_root_node.mkdir()

        wrapper_files = {
            'gradlew' : android_root,
            'gradlew.bat' : android_root,
            'gradle-wrapper.jar' : wrapper_root_node
        }

        for filename, dest_root in wrapper_files.iteritems():
            node = source_node.find_node(filename)
            if not node:
                self.fatal('[ERROR] Failed to find required Gradle wrapper file - {} - in {}'.format(filename, source_node.abspath()))

            dest_node = dest_root.make_node(filename)

            Logs.debug('android_studio: Copying %s to %s', node.abspath(), dest_node.abspath())
            shutil.copyfile(node.abspath(), dest_node.abspath())
            dest_node.chmod(511) # same as chmod 777

        wrapper_properties_node = wrapper_root_node.make_node('gradle-wrapper.properties')
        Logs.debug('android_studio: Creating %s', wrapper_properties_node.abspath())
        try:
            with open(wrapper_properties_node.abspath(), 'w') as props_file:
                write_auto_gen_header(props_file)
                props_file.write(GRADLE_WRAPPER_PROPERTIES)
        except Exception as err:
            self.fatal(str(err))

        Logs.pprint('CYAN','[INFO] Created at %s' % android_root.abspath())

    def process_module(self, project, target_name, android_root, module_type, task_generator):
        '''
        Adds the module to the project. If it's an application, then it parse the json file for
        the Android Libraries and adds those modules to the project also.
        '''
        if module_type == Module.Type.Application:
            class _DummyTaskGenerator(object):

                def set_task_attribute(self, name, attr):
                    if attr:
                        setattr(self, name, attr)

            # Generate all the targets for the Android libraries
            java_libs_json = self.root.make_node(getattr(task_generator, 'android_java_libs', []))
            json_data = self.parse_json_file(java_libs_json)
            if json_data:
                module_deps = []
                for lib_name, value in json_data.iteritems():
                    new_task_generator = _DummyTaskGenerator()
                    # Check if the library was patched. If so, we need to look in a different folder.
                    if 'patches' in value:
                        lib_path = os.path.join(self.Path(self.get_android_patched_libraries_relative_path()), lib_name)
                    else:
                        # Search the multiple library paths where the library can be.
                        lib_path = None
                        for path in value['srcDir']:
                            path = Template(path).substitute(self.env)
                            path = to_gradle_path(path)

                            if os.path.exists(path):
                                lib_path = path
                                break

                    if not lib_path:
                        paths = [ Template(path).substitute(self.env) for path in value['srcDir'] ]
                        self.fatal('[ERROR] Failed to find library - {} - in path(s): {}.  Please '
                                    'download the library from the Android SDK Manager and run the '
                                    'configure command again.'.format(lib_name, ", ".join(paths)))

                    new_task_generator.set_task_attribute('path', self.path)
                    new_task_generator.set_task_attribute('android_java_src_path', os.path.join(lib_path, 'src'))
                    new_task_generator.set_task_attribute('android_res_path', os.path.join(lib_path, 'res'))
                    new_task_generator.set_task_attribute('android_manifest_path', os.path.join(lib_path, 'AndroidManifest.xml'))
                    new_task_generator.set_task_attribute('module_dependencies', value.get('dependencies', None))
                    if value.get('launcherDependency'):
                        module_deps.append(lib_name)

                    if value.get('libs'):
                        # Get any java libs that are needed
                        file_deps = []
                        for java_lib in value['libs']:
                            file_path = Template(java_lib['path']).substitute(self.env)
                            file_path = to_gradle_path(file_path)

                            if os.path.exists(file_path):
                                file_deps.append(file_path)
                            elif java_lib['required']:
                                self.fatal('[ERROR] Required java lib - {} - was not found'.format(file_path))

                        new_task_generator.set_task_attribute('file_dependencies', file_deps)

                    project.add_target_to_project(lib_name, Module.Type.Library, new_task_generator)

                setattr(task_generator, 'module_dependencies', module_deps)

        project.add_target_to_project(target_name, module_type, task_generator)

