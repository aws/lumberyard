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

import os, sys, shutil, random, time, re, stat, string

import xml.etree.ElementTree as ET

from collections import defaultdict
from pprint import pprint, pformat
from string import maketrans
from StringIO import StringIO

from waflib import Node, Context, TaskGen, Build, Utils, Logs
from waflib.Configure import conf
from waflib.TaskGen import feature, after_method

from waf_branch_spec import CONFIGURATIONS

################################################################
#                     Defaults                                 #
PROJECT_BUILD_TEMPLATE = r'''
apply from: "waf.gradle"

buildscript {
    repositories {
        jcenter()
    }
    dependencies {
        classpath "com.android.tools.build:gradle-experimental:0.9.3"

        // NOTE: Do not place your application dependencies here: they belong
        // in the individual module build.gradle files
    }
}

allprojects { project ->
    buildDir "${binTempRoot}/${rootProject.name}/${project.name}"
    repositories {
        jcenter()
    }
}
'''

WAF_GRADLE_TASK = r'''
import org.apache.tools.ant.taskdefs.condition.Os

// the WAF gradle task wrapper
class WafTask extends Exec {
    WafTask() {
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

// export these closures as common waf properties
project.ext.WafTask = WafTask
'''

TASK_GEN_HEADER = r'''
// generate all the build tasks
afterEvaluate {

    // disable the built in native tasks
    tasks.whenTaskAdded { task ->
        if (task.name.contains("Shared") || task.name.contains("Static"))
        {
            task.deleteAllActions()
        }
    }

    // add the waf build tasks to the build chain
    project.ext.platforms.each { platform ->
        project.ext.configurations.each { config ->

            String targetName = "${platform.capitalize()}${config.capitalize()}"

            // create the custom waf task
            String wafTaskName = "build${targetName}Waf"
            String commandArgs = "build_${platform}_${config} -p all %s"

            tasks.create(name: wafTaskName, type: WafTask, description: "lmbr_waf ${commandArgs}") {
                doFirst {
                    args commandArgs.split(" ")
                }
            }

            // add the waf task to the build chain
            String compileJavaTask = "compile${targetName}Sources"
            tasks.whenTaskAdded { task ->
                if (task.name == compileJavaTask) {
                    task.dependsOn wafTaskName
                }
            }
'''

COPY_TASK_GEN = r'''
            // generate the copy apk task
            String copyTaskName = "copy${platform.capitalize()}${config.capitalize()}Apk"
            tasks.create(name: copyTaskName, type: Copy){
                from "${buildDir}/outputs/apk"
                into "${engineRoot}/${androidBinMap[platform][config]}"
                include "${project.name}-${platform}-${config}.apk"

                rename { String fileName ->
                    fileName.replace("-${platform}-${config}", "")
                }
            }

            // add the copy apk task to the build chain
            String assembleTask = "assemble${targetName}"
            tasks.whenTaskAdded { task ->
                if (task.name == assembleTask) {
                    task.finalizedBy copyTaskName
                }
            }
'''

TASK_GEN_FOOTER = r'''
        }
    }
}
'''

GRADLE_PROPERTIES = r'''
################################################################
# This file was automatically created by WAF
# WARNING! All modifications will be lost!
################################################################

# Android Studio project settings overrides

# For more details on how to configure your build environment visit
# http://www.gradle.org/docs/current/userguide/build_environment.html

# Enable Gradle as a daemon to improve the startup and execution time
org.gradle.daemon=true

# Since Lumberyard is a large project, we need to override the JVM daemon process memory settings
# Defaults -Xmx10248m -XX:MaxPermSize=256m
org.gradle.jvmargs=-Xmx2048m -XX:MaxPermSize=512m

# Enable the new (incubating) selective Gradle configure mode.  This should help improve
# build time due to the large size of Lumberyard.
# http://www.gradle.org/docs/current/userguide/multi_project_builds.html#sec:configuration_on_demand
org.gradle.configureondemand=true
'''

GRADLE_WRAPPER_PROPERTIES = r'''
################################################################
# This file was automatically created by WAF
# WARNING! All modifications will be lost!
################################################################
distributionBase=GRADLE_USER_HOME
distributionPath=wrapper/dists
zipStoreBase=GRADLE_USER_HOME
zipStorePath=wrapper/dists
distributionUrl=https\://services.gradle.org/distributions/gradle-3.3-all.zip
'''

GRADLE_WRAPPER_DIR = 'Code/Launcher/AndroidLauncher/ProjectBuilder/GradleWrapper'
#                     Defaults END                             #
################################################################


################################################################
def debug_log_value(key_word, value):
    Logs.debug('android_studio: -- %32s: %s', key_word, value)


################################################################
def inject_auto_gen_header(writer):
    writer('////////////////////////////////////////////////////////////////\n')
    writer('// This file was automatically created by WAF\n')
    writer('// WARNING! All modifications will be lost!\n')
    writer('////////////////////////////////////////////////////////////////\n')


################################################################
def find_common_path(paths_list):
    root_path = os.path.commonprefix(paths_list)
    if not os.path.exists(root_path) or not os.path.isdir(root_path):
        root_path = root_path[:root_path.rindex(os.sep)]
    return root_path


################################################################
def get_android_sdk_version_number(conf):
    sdk_version = conf.get_android_sdk_version()
    version_tokens = sdk_version.split('-')
    if version_tokens[1].isdigit():
        return int(version_tokens[1])
    else:
        return 19 # fall back to the lowest version of android we support


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
            output = value
        else:
            output += value.title()

    return output


################################################################
def indent_text(text, indent, stream):
    indent_space = ' ' * indent * 4

    result = ''
    for line in text.splitlines():
        result += "%s%s\n" % (indent_space, line)

    stream.write(result)


################################################################
class GradleList(list):
    """
    Wrapped list to determine when to use special formating
    """
    def __init__(self, *k):
        for elem in k:
            self.append(elem)


################################################################
class GradleNode(object):

    ################################
    def to_string(self, obj):
        if isinstance(obj, bool):
            return str(obj).lower()

        elif isinstance(obj, str):
             # convert to forward slash because on windows requires a double back slash
             # to be print but it only prints one in the string
            return '"%s"' % obj.replace('\\', '/')

        elif isinstance(obj, GradleList):
            if len(obj) == 1:
                return self.to_string(obj[0])
            else:
                return pformat(obj).replace("'", '"')

        else:
            return str(obj)

    ################################
    def write_value(self, obj, stream, indent = 0):
        next_indent = indent + 1

        if isinstance(obj, GradleNode):
            obj.write(stream, indent)

        elif isinstance(obj, dict):
            for key, value in obj.items():
                indent_text('%s %s' % (convert_to_gradle_name(key), self.to_string(value)), next_indent, stream)

    ################################
    def write_internal_dict(self, stream, indent = 0):
        next_indent = indent + 1
        hidden_attrib_prefix = '_%s_' % self.__class__.__name__

        for attribute, value in self.__dict__.items():

            if not value or attribute.startswith(hidden_attrib_prefix):
                continue

            elif isinstance(value, GradleNode):
                value.write(stream, next_indent)

            elif isinstance(value, dict):
                for key, subvalue in value.items():
                    if not subvalue:
                        continue

                    if hasattr(subvalue, 'gradle_name'):
                        self.write_value(subvalue, stream, next_indent)
                    else:
                        indent_text('%s {' % key, next_indent, stream)
                        self.write_value(subvalue, stream, next_indent)
                        indent_text('}', next_indent, stream)

            elif isinstance(value, GradleList):
                format = '%s.add(%s)' if len(value) == 1 else '%s.addAll(%s)'
                indent_text(format % (convert_to_gradle_name(attribute), self.to_string(value)), next_indent, stream)

            elif isinstance(value, list):
                for elem in value:
                    indent_text('%s %s' % (convert_to_gradle_name(attribute), self.to_string(elem)), next_indent, stream)

            else:
                indent_text('%s %s' % (convert_to_gradle_name(attribute), self.to_string(value)), next_indent, stream)

    ################################
    def write(self, stream, indent = 0):

        if hasattr(self.__class__, 'gradle_name'):
            indent_text('%s {' % self.__class__.gradle_name, indent, stream)
            self.write_internal_dict(stream, indent)
            indent_text('}', indent, stream)

        else:
            self.write_internal_dict(stream, indent)


################################################################
class DefaultConfig(GradleNode):
    gradle_name = 'defaultConfig'

    ################################
    class ApiLevel(GradleNode):

        ################################
        def __init__(self, api_level = 0):
            self.api_level = api_level

        ################################
        def __nonzero__(self):
            return self.api_level > 0

    ################################
    def __init__(self):
        self.application_id = ''
        self.sdk_versions = defaultdict(DefaultConfig.ApiLevel)

    ################################
    def __nonzero__(self):
        if self.application_id or self.sdk_versions:
            return True
        else:
            return False

    ################################
    def set_properties(self, **props):
        if 'application_id' in props:
            self.application_id = props['application_id']

        if 'min_sdk' in props:
            self.sdk_versions['minSdkVersion'].api_level = props['min_sdk']

        if 'target_sdk' in props:
            self.sdk_versions['targetSdkVersion'].api_level = props['target_sdk']


################################################################
class SigningConfigRef:
    """
    Wrapper class to provide custom formating
    """

    ################################
    def __init__(self, config_name = ''):
        self.config_name = config_name

    ################################
    def __nonzero__(self):
        return True if self.config_name else False

    ################################
    def __str__(self):
        return '= $("android.signingConfigs.%s")' % self.config_name


################################################################
class SigningConfigs(GradleNode):
    """
    This has to be outside the android block in the model definition in the experimental version
    of gradle we are using.  However, it is still part of the android properties.
    """
    gradle_name = 'android.signingConfigs'

    ################################
    class Config(GradleNode):

        ################################
        def __init__(self):
            self.key_alias = ''
            self.key_password = ''
            self.store_file = ''
            self.store_password = ''

        ################################
        def __nonzero__(self):
            if self.key_alias and self.key_password and self.store_file and self.store_password:
                return True
            else:
                return False

    ################################
    def __init__(self):
        self.configs = defaultdict(SigningConfigs.Config)

    ################################
    def __nonzero__(self):
        for value in self.configs:
            if value:
                return True
        return False

    ################################
    def add_signing_config(self, config_name, **config_props):
        config = SigningConfigs.Config()

        for attribute in config.__dict__.keys():
            config.__dict__[attribute] = config_props.get(attribute, '')

        config_name = 'create("%s")' % config_name
        self.configs[config_name] = config


################################################################
class NdkProperties(GradleNode):
    gradle_name = 'ndk'

    ################################
    def __init__(self):
        self.module_name = ""
        self.debuggable = False
        self.abi_filters = GradleList()
        self._c_flags = GradleList()
        self.cpp_flags = GradleList()

    ################################
    def __nonzero__(self):
        if self.module_name or self.debuggable or self.abi_filters or self._c_flags or self.cpp_flags:
            return True
        else:
            return False

    ################################
    def set_module_name(self, module_name):
        self.module_name = module_name

    ################################
    def set_debuggable(self, debuggable):
        self.debuggable = debuggable

    ################################
    def add_abi_filters(self, *abi_filters):
        self.abi_filters.extend(abi_filters)

    ################################
    def add_general_compiler_flags(self, *flags):
        self._c_flags.extend(flags)
        self.cpp_flags.extend(flags)


################################################################
class Sources(GradleNode):
    gradle_name = 'sources'

    ################################
    class Paths(GradleNode):

        ################################
        def __init__(self):
            self.src_dir = []
            self.src_file = []
            self.excludes = GradleList()

        ################################
        def __nonzero__(self):
            if self.src_dir or self.src_file or self.excludes:
                return True
            else:
                return False

        ################################
        def add_src_paths(self, *paths):
            self.src_dir.extend(paths)

        ################################
        def add_src_files(self, *files):
            self.src_file.extend(files)

        ################################
        def add_excludes(self, *excludes):
            self.excludes.extend(excludes)

    ################################
    class Dependencies(GradleNode):
        gradle_name = 'dependencies'

        ################################
        def __init__(self):
            self.project = []

        ################################
        def __nonzero__(self):
            return True if self.project else False

        ################################
        def add_projects(self, *projects):
            deps = [':%s' % proj if proj[0] != ':' else proj for proj in projects]
            self.project.extend(deps)

        ################################
        def clear_projects(self, *projects):
            del self.project[:]

    ################################
    class Set(GradleNode):

        ################################
        def __init__(self):
            self.paths = defaultdict(Sources.Paths)
            self.dependencies = Sources.Dependencies()

        ################################
        def __nonzero__(self):
            if self.paths:
                for key, value in self.paths.items():
                    if value:
                        return True

            if self.dependencies:
                return True
            else:
                return False

        ################################
        def add_export_paths(self, *paths):
            self.paths['exportedHeaders'].add_src_paths(*paths)

        ################################
        def add_src_paths(self, *paths):
            self.paths['source'].add_src_paths(*paths)

        ################################
        def add_project_dependencies(self, *projects):
            self.dependencies.add_projects(*projects)

        ################################
        def clear_project_dependencies(self):
            self.dependencies.clear_projects()

    ################################
    class SourceTypes(GradleNode):

        ################################
        def __init__(self):
            self.sets = defaultdict(Sources.Set)

        ################################
        def __nonzero__(self):
            for value in self.sets.values():
                if value:
                    return True
            return False

        ################################
        def set_java_properties(self, **props):
            if 'java_src' in props:
                java_props = self.sets['java']
                src = to_list(props['java_src'])
                java_props.add_src_paths(*src)

            if 'aidl_src' in props:
                aidl_props = self.sets['aidl']
                aidl_src = to_list(props['aidl_src'])
                aidl_props.add_src_paths(*aidl_src)

            if 'res_src' in props:
                android_resources = self.sets['res']
                res = to_list(props['res_src'])
                android_resources.add_src_paths(*res)

            if 'manifest_path' in props:
                android_manifest = self.sets['manifest']
                manifest = to_list(props['manifest_path'])
                android_manifest.add_src_paths(*manifest)

        ################################
        def get_jni_dependencies(self):
            jni_props = self.sets['jni']
            return jni_props.dependencies.project

        ################################
        def set_jni_dependencies(self, dependencies):
            jni_props = self.sets['jni']
            jni_props.clear_project_dependencies()
            jni_props.add_project_dependencies(*dependencies)

        ################################
        def set_jni_properties(self, **props):
            jni_props = self.sets['jni']

            if 'jni_exports' in props:
                exports = to_list(props['jni_exports'])
                jni_props.add_export_paths(*exports)

            if 'jni_src' in props:
                src = to_list(props['jni_src'])
                jni_props.add_src_paths(*src)

            if 'jni_dependencies' in props:
                deps = to_list(props['jni_dependencies'])
                jni_props.add_project_dependencies(*deps)

    ################################
    def __init__(self):
        self.variants = defaultdict(Sources.SourceTypes)

    ################################
    def __nonzero__(self):
        for value in self.variants.values():
            if value:
                return True
        return False

    ################################
    def set_main_properties(self, **props):
        self.set_variant_properties('main', **props)

    ################################
    def set_variant_properties(self, variant_name, **props):
        variant = self.variants[variant_name]

        variant.set_java_properties(**props)
        variant.set_jni_properties(**props)

    ################################
    def validate_and_set_main_dependencies(self, *dependencies):
        self.validate_and_set_variant_dependencies('main', *dependencies)

    ################################
    def validate_and_set_variant_dependencies(self, variant_name, dependencies):
        variant = self.variants[variant_name]

        current_deps = variant.get_jni_dependencies()
        valid_deps = [project for project in current_deps if project[1:] in dependencies]

        variant.set_jni_dependencies(valid_deps)


################################################################
class Builds(GradleNode):
    gradle_name = 'buildTypes'

    ################################################################
    class Type(GradleNode):

        ################################
        def __init__(self):
            self.ndk = NdkProperties()
            self.debuggable = False
            self.signing_config = SigningConfigRef()

        ################################
        def set_debuggable(self, debuggable):
            self.debuggable = debuggable
            self.ndk.set_debuggable(debuggable)

        ################################
        def add_ndk_compiler_flags(self, *flags):
            self.ndk.add_general_compiler_flags(*flags)

    ################################
    def __init__(self):
        self.types = defaultdict(Builds.Type)

    ################################
    def __nonzero__(self):
        return True if self.types else False

    ################################
    def add_build_type(self, build_name, **build_props):
        if build_name not in ['debug', 'release']:
            build_name = 'create("%s")' % build_name

        debuggable = False
        if 'debug' in build_name:
            debuggable = True

        build_type = self.types[build_name]
        build_type.set_debuggable(build_props.get('debuggable', debuggable))

        if 'signing_config_ref' in build_props:
            build_type.signing_config = SigningConfigRef(build_props['signing_config_ref'])

        if 'ndk_flags' in build_props:
            flags = to_list(build_props['ndk_flags'])
            build_type.add_ndk_compiler_flags(*flags)


################################################################
class Products(GradleNode):
    gradle_name = 'productFlavors'

    ################################################################
    class Flavor(GradleNode):

        ################################
        def __init__(self):
            self.ndk = NdkProperties()

        ################################
        def add_abi_filters(self, *abis):
            self.ndk.add_abi_filters(*abis)

        ################################
        def add_ndk_compiler_flags(self, *flags):
            self.ndk.add_general_compiler_flags(*flags)

    ################################
    def __init__(self):
        self.flavors = defaultdict(Products.Flavor)

    ################################
    def __nonzero__(self):
        return True if self.flavors else False

    ################################
    def add_product_flavor(self, flavor_name, **flavor_props):
        key_name = 'create("%s")' % flavor_name

        product_flavor = self.flavors[key_name]

        if 'abis' in flavor_props:
            abis = to_list(flavor_props['abis'])
            product_flavor.add_abi_filters(*abis)

        if 'ndk_flags' in flavor_props:
            flags = to_list(flavor_props['ndk_flags'])
            product_flavor.add_ndk_compiler_flags(*flags)


################################################################
class Android(GradleNode):
    gradle_name = 'android'

    ################################
    def __init__(self):
        self.compile_sdk_version = 19 # default value is lowest version we support
        self.build_tools_version = ''
        self.default_config = DefaultConfig()
        self.ndk = NdkProperties()
        self.build_types = Builds()
        self.product_flavors = Products()
        self.sources = Sources()

    ################################
    def set_general_properties(self, **props):
        if 'module_name' in props:
            self.ndk.set_module_name(props['module_name'])

        if 'sdk_version' in props:
            self.compile_sdk_version = props['sdk_version']

        if 'build_tools_version' in props:
            self.build_tools_version = props['build_tools_version']

        self.default_config.set_properties(**props)

    ################################
    def set_ndk_properties(self, **props):
        if 'module_name' in props:
            self.ndk.set_module_name(props['module_name'])

        if 'ndk_flags' in props:
            flags = to_list(props['ndk_flags'])
            self.ndk.add_general_compiler_flags(*flags)

    ################################
    def add_build_type(self, build_name, **props):
        self.build_types.add_build_type(build_name, **props)

    ################################
    def add_product_flavor(self, flavor_name, **props):
        self.product_flavors.add_product_flavor(flavor_name, **props)

    ################################
    def set_main_source_paths(self, **paths):
        self.sources.set_main_properties(**paths)

    ################################
    def validate_and_set_main_dependencies(self, dependencies):
        self.sources.validate_and_set_main_dependencies(dependencies)

################################################################
class ModelType:
    Native_lib, Application, Android_lib = range(3)

################################################################
class Model(GradleNode):
    gradle_name = 'model'

    ################################
    def __init__(self, parent):
        self.android = Android()
        self.signing_configs = SigningConfigs()
        self.__parent = parent

    ################################
    def apply_platform_configs(self, ctx, platform_abis, platform_defines, configs):
        if self.__parent.type == ModelType.Android_lib:
            return

        android = self.android

        if self.__parent.type == ModelType.Application:
            signing_props = {
                'key_alias' : ctx.get_android_dev_keystore_alias(),
                'store_file' : ctx.get_android_dev_keystore_path(),
                'key_password' : ctx.options.dev_key_pass,
                'store_password' : ctx.options.dev_store_pass
            }

            self.signing_configs.add_signing_config('Development', **signing_props)

        for platform, abi in platform_abis.items():
            common_platform_defines = platform_defines[platform]

            for config in configs:
                env_name = '%s_%s' % (platform, config)
                env = ctx.all_envs[env_name]

                build_props = {}

                all_config_defines = env['DEFINES']
                filtered_defines = [define for define in all_config_defines if '"' not in define and define not in common_platform_defines]

                # add the defines so the symbol resolution can happen in the editor
                build_props['ndk_flags'] = ['-D%s' % define for define in filtered_defines]

                if self.__parent.type == ModelType.Application:
                    build_props['signing_config_ref'] = 'Development'

                android.add_build_type(config, **build_props)

            flags = ['-D%s' % define for define in common_platform_defines if '"' not in define]
            android.add_product_flavor(platform, abis = abi, ndk_flags = flags)

    ################################
    def process_target(self, proj_props, target_name, task_generator):
        android = self.android
        ctx = proj_props.ctx

        package_name = ''
        if self.__parent.type == ModelType.Application:
            game_project = getattr(task_generator, 'project_name', '')
            package_name = ctx.get_android_package_name(game_project)

        target_sdk = getattr(task_generator, 'sdk_version', ctx.env['ANDROID_SDK_VERSION_NUMBER'])
        android.set_general_properties(
            sdk_version = target_sdk,
            build_tools_version = ctx.get_android_build_tools_version(),
            application_id = package_name,
            min_sdk = getattr(task_generator, 'min_sdk', ctx.env['ANDROID_NDK_PLATFORM_NUMBER']),
            target_sdk = target_sdk
        )

        def _defines_to_flags(defines_list):
            return ['-D%s' % define for define in defines_list if '"' not in define]

        if not self.__parent.type == ModelType.Android_lib:
            # process the task specific defines
            common_defines = ctx.collect_task_gen_attrib(task_generator, 'defines')
            debug_log_value('Common Defines', common_defines)

            flags = _defines_to_flags(common_defines)
            android.set_ndk_properties(
                module_name = target_name,
                ndk_flags = flags
            )

            for config in proj_props.configs:
                config_defines = ctx.collect_task_gen_attrib(task_generator, '%s_defines' % config)
                debug_log_value('{} Defines'.format(config.title()), config_defines)

                if config_defines:
                    config_flags = _defines_to_flags(config_defines)
                    android.add_build_type(config, ndk_flags = config_flags)

            for target in proj_props.target_abis.keys():
                target_defines = ctx.collect_task_gen_attrib(task_generator, '%s_defines' % target)
                debug_log_value('{} Defines'.format(target.title()), target_defines)

                if target_defines:
                    target_flags = _defines_to_flags(target_defines)
                    android.add_product_flavor(target, ndk_flags = target_flags)

        # process the source paths and native dependencies
        jni_src_paths = ctx.extrapolate_src_paths(task_generator)
        export_includes = ctx.collect_task_gen_attrib(task_generator, 'export_includes')
        jni_export_paths = []
        for exp_incl in export_includes:
            real_path = exp_incl
            if isinstance(exp_incl, Node.Node):
                real_path = exp_incl.abspath()
            elif not os.path.isabs(real_path):
                relative_path = os.path.join(task_generator.path.abspath(), exp_incl)
                real_path = os.path.normpath(relative_path)

            # having anything reference the root SDKs or Tools directory
            # will cause Android Studio to crash... :(
            if os.path.basename(real_path) not in ['SDKs', 'Tools']:
                jni_export_paths.append(real_path)

        if export_includes and not jni_export_paths:
            jni_export_paths = jni_src_paths

        # android specific path handling
        java_src_paths = []
        aidl_src_paths = []
        manifest_path = []
        resource_src_path = []

        # native only modules can't have any java / manifest / resources, so only gather them for
        # applications and android libs
        if self.__parent.type in (ModelType.Application, ModelType.Android_lib):
            java_src_paths = ctx.collect_task_gen_attrib(task_generator, 'android_java_src_path')
            aidl_src_paths = ctx.collect_task_gen_attrib(task_generator, 'android_aidl_src_path')
            manifest_path = ctx.collect_task_gen_attrib(task_generator, 'android_manifest_path')
            resource_src_path = ctx.collect_task_gen_attrib(task_generator, 'android_res_path')

        # if we are a application, pull in all the java files from dependent modules so they get
        # built correctly by android studio
        if self.__parent.type == ModelType.Application:
            game_project = getattr(task_generator, 'project_name', None)

            for tsk_gen in ctx.project_tasks:
                module_name = tsk_gen.target

                # skip the launchers / same module, those source paths were already added above
                if module_name.endswith('AndroidLauncher'):
                    continue

                if ctx.is_module_for_game_project(module_name, game_project, None):
                    java_src_paths += ctx.collect_task_gen_attrib(tsk_gen, 'android_java_src_path')
                    aidl_src_paths += ctx.collect_task_gen_attrib(tsk_gen, 'android_aidl_src_path')

        debug_log_value('Source Paths (java)', java_src_paths)
        debug_log_value('Source Paths (aidl)', aidl_src_paths)

        # for some reason get_module_uses doesn't work correctly on the
        # launcher tasks, so we need to maunally create the use dependecy
        # tree
        all_native_uses = []
        if self.__parent.type == ModelType.Application:
            local_uses = ctx.collect_task_gen_attrib(task_generator, 'use')
            all_native_uses = local_uses[:]
            for use in local_uses:
                all_native_uses.extend(ctx.get_module_uses(use, proj_props.project_spec))

        elif self.__parent.type == ModelType.Native_lib:
            all_native_uses = ctx.get_module_uses(target_name, proj_props.project_spec)

        project_names = [ tgen.target for tgen in ctx.project_tasks ]
        valid_deps = set(project_names).intersection(all_native_uses) # [project for project in current_deps if project[1:] in dependencies]
        debug_log_value('Uses (native)', list(valid_deps))

        android.set_main_source_paths(
            java_src = java_src_paths,
            aidl_src = aidl_src_paths,
            jni_src = jni_src_paths,
            jni_exports = jni_export_paths,
            jni_dependencies = valid_deps,
            manifest_path = manifest_path,
            res_src = resource_src_path
        )

    ################################
    def validate_and_set_project_dependencies(self, project_modules):
        self.android.validate_and_set_main_dependencies(project_modules)

################################
class GradleContainer(GradleNode):
    ################################
    def __init__(self):
        self.nodes = {}

    ################################
    def __getattr__(self, name):
        return self.nodes[name]

    ################################
    def write(self, stream, indent = 0):
        return super(GradleContainer, self).write(stream, indent - 1)

################################
class Module(GradleContainer):
    '''
    Represents a Module in Android Studio
    '''
    ################################
    def __init__(self, type):
        super(Module, self).__init__()
        self.__type = type

    ################################
    def write(self, stream, indent = 0):
        indent_text('apply from: "../waf.gradle"', indent, stream)

        if self.__type == ModelType.Application:
            plugin__type = 'application'
        elif self.__type == ModelType.Android_lib:
            plugin__type = 'library'
        else:
            plugin__type = 'native'

        indent_text('apply plugin: "com.android.model.%s"\n\n' % plugin__type, indent, stream)
        super(Module, self).write(stream, indent)
        stream.write('\n')

    ################################
    def process_module_dependencies(self, project, task_generator):
        deps = ModuleDependencies()
        addNode = False

        modules = project.ctx.collect_task_gen_attrib(task_generator, 'modules_dependency')
        if modules:
            deps.add_compile_projects(*modules)
            addNode = True

        files = project.ctx.collect_task_gen_attrib(task_generator, 'files_dependency')
        if files:
            deps.add_compile_files(*files)
            addNode = True

        # Look for the android specific uses
        if self.__type == ModelType.Application:
            def _get_task_gen(task_name):
                try:
                    return project.ctx.get_tgen_by_name(task_name)
                except:
                    return None

            launcher_name = getattr(task_generator, 'project_name', '')
            apk_task_name = '{}_APK'.format(launcher_name)

            apk_task = _get_task_gen(apk_task_name)
            if apk_task:
                uses_added = []

                uses = project.ctx.collect_task_gen_attrib(apk_task, 'use')
                for use in uses:
                    use_task_gen = _get_task_gen(use)
                    if use_task_gen:
                        use_task_gen.post()

                        if not hasattr(use_task_gen, 'aar_task'):
                            continue

                        android_studio_name = getattr(use_task_gen, 'android_studio_name', None)
                        if android_studio_name:
                            deps.add_compile_libraries(android_studio_name)
                            uses_added.append(android_studio_name)
                        else:
                            deps.add_compile_projects(use)
                            uses_added.append(use)

                        addNode = True
                debug_log_value('Uses (android)', uses_added)

        if addNode:
            self.addNode(deps, deps.gradle_name)

    ################################
    def addNode(self, node, name):
        self.nodes[name] = node

    ################################
    @property
    def type(self):
        return self.__type

    ################################
    @type.setter
    def type(self, value):
        self.__type = value

    ################################
    @type.deleter
    def type(self):
        del self.__type

################################
class ModuleDependencies(GradleNode):
    '''
    Represents a Module dependency
    '''
    gradle_name = 'dependencies'

    ################################
    class _Dependency:
        '''
        Helper class to write the dependency in the proper way.
        '''
        ################################
        def __init__(self, name, type = None):
            self.name = name
            self.type = type

        ################################
        def __str__(self):
            if self.type:
                return '{}("{}")'.format(self.type, self.name)
            else:
                return '"{}"'.format(self.name)

        ################################
        def __nonzero__(self):
            return any([self.name, self.type])

    ################################
    def __init__(self):
        self.compile = []

    ################################
    def __nonzero__(self):
        return True if self.compile else False

    ################################
    def clear_compile_projects(self):
        del self.compile[:]

    ################################
    def add_compile_projects(self, *projects):
        deps = [self._Dependency(':' + proj, 'project') for proj in projects]
        self.compile.extend(deps)

    ################################
    def add_compile_libraries(self, *libraries):
        deps = [self._Dependency(lib) for lib in libraries]
        self.compile.extend(deps)

    ################################
    def add_compile_files(self, *files):
        deps = [self._Dependency(file, 'files') for file in files]
        self.compile.extend(deps)

################################################################
class AndroidStudioProject:

    ################################
    def __init__(self, ctx):
        self.projects = {}
        self.configs = []
        self.target_abis = {}
        self.target_defines = {}
        self.project_spec = ''
        self.ctx = ctx

    ################################
    def set_project_spec(self, project_spec):
        self.project_spec = project_spec

    ################################
    def set_platforms_and_configs(self, ctx, target_abis, configs):
        self.configs = configs
        self.target_abis = target_abis

        # collect all the configuration defines and filter out the
        # common ones based on the target platform
        for target, abi in target_abis.items():
            config_lists = []
            for config in configs:
                env_name = '%s_%s' % (target, config)
                env = ctx.all_envs[env_name]

                config_lists.append(env['DEFINES'])

            self.target_defines[target] = set(config_lists[0]).intersection(*config_lists[1:])

    ################################
    def add_target_to_project(self, project_name, type, project_task_gen):
        Logs.debug('android_studio: Added Module - {} - to project'.format(project_name))

        android_module = Module(type)
        android_model = Model(android_module)

        android_model.apply_platform_configs(self.ctx, self.target_abis, self.target_defines, self.configs)
        android_model.process_target(self, project_name, project_task_gen)

        android_module.addNode(android_model, android_model.gradle_name)
        android_module.process_module_dependencies(self, project_task_gen)

        self.projects[project_name] = android_module

    ################################
    def write_project(self, root_node):
        # generate the include file for gradle
        gradle_settings = root_node.make_node('settings.gradle')
        added_targets = sorted(self.projects.keys())
        try:
            with open(gradle_settings.abspath(), 'w') as settings_file:
                inject_auto_gen_header(settings_file.write)

                for target in added_targets:
                    settings_file.write('include ":%s"\n' % target)
        except Exception as err:
            self.ctx.fatal(str(err))

        # generate all the project build.gradle files
        for target_name, project in self.projects.items():
            project.model.validate_and_set_project_dependencies(added_targets)

            module_dir = root_node.make_node(target_name)
            module_dir.mkdir()

            module_node = module_dir.make_node('build.gradle')
            try:
                with open(module_node.abspath(), 'w') as build_file:
                    inject_auto_gen_header(build_file.write)
                    project.write(build_file)

                    if not project.type == ModelType.Android_lib:
                        if project.type == ModelType.Native_lib:
                            build_file.write(TASK_GEN_HEADER % '--targets=${project.name}')
                        else:
                            cmd_arg = '--enabled-game-projects=%s' % target_name.replace('Launcher', '')
                            build_file.write(TASK_GEN_HEADER % cmd_arg)
                            build_file.write(COPY_TASK_GEN)

                        build_file.write(TASK_GEN_FOOTER)
            except Exception as err:
                self.ctx.fatal(str(err))


################################################################
################################################################
def options(opt):
    group = opt.add_option_group('android-studio config')

    # disables the apk packaging process so android studio can do it
    group.add_option('--from-android-studio', dest = 'from_android_studio', action = 'store_true', default = False, help = 'INTERNAL USE ONLY for Experimental Andorid Studio support')


################################################################
class android_studio(Build.BuildContext):
    cmd = 'android_studio'

    ################################
    def get_target_platforms(self):
        """
        Used in cryengine_modules get_platform_list during project generation
        """
        android_targets = [ target for target in self.get_supported_platforms() if 'android' in target ]
        return [ 'android' ] + android_targets

    ################################
    def collect_task_gen_attrib(self, task_generator, attribute, *modifiers):
        """
        Helper for getting an attribute from the task gen with optional modifiers
        """
        result = to_list(getattr(task_generator, attribute, []))
        for mod in modifiers:
            mod_attrib = '%s_%s' % (mod, attribute)
            result.extend(to_list(getattr(task_generator, mod_attrib, [])))
        return result

    ################################
    def flatten_tree(self, path_root, obj):
        """
        Helper for getting all the leaf nodes in a dictionary
        """
        elems = []
        if isinstance(obj, dict):
            for key, value in obj.items():
                elems.extend(self.flatten_tree(path_root, value))
        elif isinstance(obj, list):
            for value in obj:
                elems.extend(self.flatten_tree(path_root, value))
        else:
            file_path = os.path.join(path_root, obj)
            elems.append(os.path.normpath(file_path))
        return elems

    ################################
    def collect_source_from_file_list(self, path_node, file_list):
        """
        Wrapper for sourcing the file list json data and flattening the
        source files into a list containing files with absolute paths
        """
        file_node = path_node.find_node(file_list)
        src_json = self.parse_json_file(file_node)
        return self.flatten_tree(path_node.abspath(), src_json)

    ################################
    def extrapolate_src_paths(self, task_generator):
        """
        By using the file lists independently along with some additional filtering
        we can make a fairly accurate guess at the root directories to include in
        the project at the narrowist scope possible. This is important due to Android
        Studio blind file tree includes for native source and will choke on large
        directory structures.
        """
        file_lists = self.collect_task_gen_attrib(task_generator, 'file_list')
        file_lists = set(file_lists) # remove the duplicates, only seems to happen for some modules and only the file_list

        exec_path = task_generator.path.abspath()

        include_paths = []
        for file_list in file_lists:
            debug_log_value('File List (native)', file_list)

            src_files = self.collect_source_from_file_list(task_generator.path, file_list)
            paths_to_add = []

            # first process the raw source, no external files
            external_filter = ['SDKs', 'Tools']
            normal_src = [src_path for src_path in src_files if not any(sub_dir for sub_dir in external_filter if sub_dir in src_path)]
            if normal_src:
                common_path = find_common_path(normal_src)
                debug_log_value('Source Path (native)', common_path)

                # attempt to determine which path is the narrower scope
                if common_path in exec_path:
                    paths_to_add.append(exec_path)
                else:
                    paths_to_add.append(common_path)

            # next process the files from SDKs to narrow their include directory
            sdk_src = [src_path for src_path in src_files if 'SDKs' in src_path]
            if sdk_src:
                common_path = find_common_path(sdk_src)
                paths_to_add.append(common_path)
                debug_log_value('SDK Path (native)', common_path)

            # same process as the SDKs for the files from Tools
            tools_src = [src_path for src_path in src_files if 'Tools' in src_path]
            if tools_src:
                common_path = find_common_path(tools_src)
                paths_to_add.append(common_path)
                debug_log_value('Tool Path (native)', common_path)

            # filter the possible paths to add again to prevent adding duplicate paths
            # or sub directories to paths already added.
            for add_path in paths_to_add:
                # This check prevents the widening of the source search path for
                # android studio.  We need the narrowist scope possible to ensure
                # duplicates don't get includes across modules and to prevent
                # android studio from crashing for tryng to parse larget directories
                # like Code/SDKs and Code/Tools
                if not (  any(incl for incl in include_paths if add_path in incl)
                       or any(incl for incl in include_paths if incl in add_path)):
                    include_paths.append(add_path)

        debug_log_value('Common Paths (native)', include_paths)
        return include_paths

    ################################
    def execute(self):
        """
        Entry point of the project generation
        """
        # restore the environments
        self.restore()
        if not self.all_envs:
            self.load_envs()

        self.load_user_settings()
        Logs.info("[WAF] Executing 'android_studio' in '%s'" % (self.variant_dir) )
        self.recurse([ self.run_dir ])

        # check the apk signing environment
        if self.get_android_build_environment() == 'Distribution':
            Logs.warn('[WARN] The Distribution build environment is not currently supported in Android Studio, falling back to the Development build environment.')

        # create the root project directory
        android_root = self.path.make_node(self.get_android_project_relative_path())
        if not os.path.exists(android_root.abspath()):
            Logs.error('[ERROR] Base android projects not generated.  Re-run the configure command.')
            return

        # generate the root gradle build script
        root_build = android_root.make_node('build.gradle')
        try:
            with open(root_build.abspath(), 'w') as root_build_file:
                inject_auto_gen_header(root_build_file.write)
                root_build_file.write(PROJECT_BUILD_TEMPLATE)
        except Exception as err:
            self.fatal(str(err))

        # get the core build settings
        project = AndroidStudioProject(self)
        android_platforms = [platform for platform in self.get_supported_platforms() if 'android' in platform]
        android_config_sets = []
        for platform in android_platforms:
            android_config_sets.append(set(config for config in self.get_supported_configurations(platform) if not config.endswith('_dedicated')))
        android_configs = list(set.intersection(*android_config_sets))

        android_abis = {}
        for android_target in android_platforms:
            abi_func = getattr(self, 'get_%s_target_abi' % android_target, None)
            if abi_func:
                android_abis[android_target] = abi_func()

        project.set_platforms_and_configs(self, android_abis, android_configs)

        # collect all the modules
        modules = []

        project_spec = (self.options.project_spec if self.options.project_spec else 'all')
        project.set_project_spec(project_spec)

        modules = self.spec_modules(project_spec)[:]
        for project_name in self.get_enabled_game_project_list():
            modules.extend(self.project_and_platform_modules(project_name, android_platforms))

        acceptable_platforms = ['android', 'all'] + android_platforms

        # first find all the modules that are going to be added to the android studio project
        tasks_to_add = []
        for group in self.groups:
            for task_generator in group:
                if not isinstance(task_generator, TaskGen.task_gen):
                    continue

                target_name = task_generator.target
                task_platforms = self.get_module_platforms(target_name)

                if target_name not in modules:
                    Logs.debug('android_studio: Skipped Module - %s - is not part of the spec' % target_name)
                    continue

                if not any(target in task_platforms for target in acceptable_platforms):
                    Logs.debug('android_studio: Skipped Module - %s - is not targeted for Andorid. Targeted for %s' % (target_name, task_platforms))
                    continue

                # remove the non-AndroidLaunchers from the project
                if target_name.endswith('Launcher') and not target_name.endswith('AndroidLauncher'):
                    Logs.debug('android_studio: Skipped Module - %s - is not an AndroidLauncher' % target_name)
                    continue

                # filter out incorrectly configured game projects and their respective launchers
                game_project = getattr(task_generator, 'project_name', target_name)
                if game_project in self.get_enabled_game_project_list():

                    if not self.get_android_settings(game_project):
                        Logs.debug('android_studio: Skipped Module - %s - is a game project not configured for Android' % target_name)
                        continue

                tasks_to_add.append(task_generator)

        self.project_tasks = tasks_to_add

        # process the modules to be added to the project
        for task_generator in tasks_to_add:
            target_name = task_generator.target

            game_project = getattr(task_generator, 'project_name', target_name)
            module_type = ModelType.Native_lib

            if game_project in self.get_enabled_game_project_list() and game_project is not target_name:
                target_name = self.get_executable_name(game_project)
                module_type = ModelType.Application

            task_generator.post()
            self.process_module(project, target_name, android_root, module_type, task_generator)

        project.write_project(android_root)

        # generate the gradle waf support file
        waf_tools = android_root.make_node('waf.gradle')
        try:
            with open(waf_tools.abspath(), 'w') as waf_tools_file:
                inject_auto_gen_header(waf_tools_file.write)
                waf_tools_file.write(WAF_GRADLE_TASK)

                indent_text('project.ext { ', 0, waf_tools_file)

                indent_text('engineRoot = "${rootDir}/%s/"' % self.path.path_from(android_root).replace('\\', '/'), 1, waf_tools_file)
                indent_text('binTempRoot = "${engineRoot}/BinTemp/"', 1, waf_tools_file)

                platforms_string = pformat(android_platforms).replace("'", '"')
                indent_text('platforms = %s' % platforms_string, 1, waf_tools_file)

                configs_string = pformat(android_configs).replace("'", '"')
                indent_text('configurations = %s' % configs_string, 1, waf_tools_file)

                indent_text('androidBinMap = [', 1, waf_tools_file)
                for platform in android_platforms:
                    indent_text('"%s" : [' % platform, 2, waf_tools_file)
                    for config in android_configs:
                        indent_text('"%s" : "%s",' % (config, self.get_output_folders(platform, config)[0]), 3, waf_tools_file)
                    indent_text('],', 2, waf_tools_file)
                indent_text(']', 1, waf_tools_file)

                indent_text('} ', 0, waf_tools_file)
        except Exception as err:
            self.fatal(str(err))

        # generate the android local properties file
        local_props = android_root.make_node('local.properties')
        try:
            with open(local_props.abspath(), 'w') as props_file:
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
                props_file.write(GRADLE_WRAPPER_PROPERTIES)
        except Exception as err:
            self.fatal(str(err))

        Logs.pprint('CYAN','[INFO] Created at %s' % android_root.abspath())

    ################################
    def process_module(self, project, target_name, android_root, module_type, task_generator):
        '''
        Adds the module to the project. If it's an application, then it parse the json file for
        the Android Libraries and adds those modules to the project also.
        '''
        if module_type == ModelType.Application:
            class _DummyTaskGenerator(object):

                def set_task_attribute(self, name, attr):
                    if attr:
                        setattr(self, name, attr)

            # Generate all the targets for the Android libraries
            java_libs_json = self.root.make_node(getattr(task_generator, 'android_java_libs', []))
            json_data = self.parse_json_file(java_libs_json)
            if json_data:
                app_dependancy = []
                for libName, value in json_data.iteritems():
                    new_task_generator = _DummyTaskGenerator()
                    # Check if the library was patched. If so, we need to look in a different folder.
                    if 'patches' in value:
                        lib_path = os.path.join(project.ctx.Path(project.ctx.get_android_patched_libraries_relative_path()), libName)
                    else:
                        # Search the multiple library paths where the library can be.
                        lib_path = None
                        for path in value['srcDir']:
                            path = string.Template(path).substitute(self.env)
                            path = path.replace('\\', '/')

                            if os.path.exists(path):
                                lib_path = path
                                break

                    if not lib_path:
                        project.ctx.fatal('[ERROR] Failed to find library - %s - in path(s) [%s]. Please download the library from the Android SDK Manager and run the configure command again.'
                                        % (libName, ", ".join(string.Template(path).substitute(self.env) for path in value['srcDir'])))

                    new_task_generator.set_task_attribute('path', self.path)
                    new_task_generator.set_task_attribute('android_java_src_path', os.path.join(lib_path, 'src'))
                    new_task_generator.set_task_attribute('android_res_path', os.path.join(lib_path, 'res'))
                    new_task_generator.set_task_attribute('android_manifest_path', lib_path)
                    new_task_generator.set_task_attribute('modules_dependency', value.get('dependancy', None))
                    if value.get('launcherDependency'):
                        app_dependancy.append(libName)

                    if value.get('libs'):
                        # Get any java libs that are needed
                        files_dep = []
                        for java_lib in value['libs']:
                            file_path = string.Template(java_lib['path']).substitute(self.env)
                            file_path = file_path.replace('\\', '/')

                            if os.path.exists(file_path):
                                files_dep.append(file_path)
                            elif java_lib['required']:
                                project.ctx.fatal("[ERROR] Required java lib [%s] was not found" % file_path)

                        new_task_generator.set_task_attribute('files_dependency', files_dep)

                    project.add_target_to_project(libName, ModelType.Android_lib, new_task_generator)

                setattr(task_generator, 'modules_dependency', app_dependancy)

        project.add_target_to_project(target_name, module_type, task_generator)

