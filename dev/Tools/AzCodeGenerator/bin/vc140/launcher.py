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
# Certain MSVC9.0 runtime environments may be incompatible with the one we intend to use
# to invoke python, here we search for any paths that contain msvcr90.dll and remove them
# to ensure they cannot be brought into our environment - failure to do this may result
# in runtime library mismatches and/or CRT violations.
# This has to be done before anything else
# (see http://stackoverflow.com/questions/14552348/runtime-error-r6034-in-embedded-python-application)
import os
import sys
import traceback, errno

try:
    # Prevent non-sxs version of msvcr90.dll from being loaded by path environment variable.
    if 'path' in os.environ:
        os.environ['path'] = os.pathsep.join(
            path for path in os.environ['path'].split(os.pathsep)
            if "msvcr90.dll" not in map(str.lower,
                                        os.listdir(path) if os.path.isdir(path) else [])
        )
except Exception, e:
    print 'error - {}'.format(e)
    raise

# ----------------------------------------------

from az_code_gen.base import *
from azcg_extension import *
import jinja_extensions
import jinja2.exceptions
import jinja2.utils


# ---------------------------------------------------
# Prepare the Jinja2 template engine.
def load_jinja2_environment(script_paths):
    try:
        from jinja2 import Environment, FileSystemLoader, StrictUndefined, DebugUndefined
    except Exception, e:
        print 'base.py error loading jinja2 environment: {}'.format(e)
        raise

    # wrap the FileSystemLoader so we can seed it with script paths and record what it requests
    class AZCGLoader(FileSystemLoader):
        def __init__(self, scripts):
            script_paths = scripts.split(',')
            paths = []
            for script_path in script_paths:
                script_dir = os.path.dirname(script_path)
                if script_dir not in paths:
                    paths.append(script_dir)
            # feed the unique script dirs as input dirs to FileSystemLoader
            super(AZCGLoader, self).__init__(paths)

        def get_source(self, environment, template):
            for searchpath in self.searchpath:
                template_filename = os.path.join(searchpath, template)
                template_file = jinja2.utils.open_if_exists(template_filename)
                if template_file is None:
                    continue
                try:
                    contents = template_file.read().decode(self.encoding)
                finally:
                    template_file.close()

                mtime = os.path.getmtime(template_filename)

                # Record dependency to utility
                RegisterDependencyFile(template_filename)

                # Allow relative template paths to this template, if not already a search path
                template_path = os.path.dirname(template_filename)
                if template_path not in self.searchpath:
                    self.searchpath.append(template_path)

                def uptodate():
                    try:
                        return os.path.getmtime(template_filename) == mtime
                    except OSError:
                        return False
                return contents, template_filename, uptodate
            raise jinja2.exceptions.TemplateNotFound(template)

    jinja_env = Environment(loader=AZCGLoader(script_paths),
                            # strip whitespace from templates as much as possible
                            trim_blocks=True, lstrip_blocks=True,
                            # throws an exception if variables are undefined. Setting this to DebugUndefined will print the error message into the generated file instead
                            undefined=StrictUndefined,
                            extensions=[jinja_extensions.error.RaiseExtension,
                                        "jinja2.ext.do"])

    jinja_extensions.template.registerExtensions(jinja_env)

    return jinja_env


# ---------------------------------------------------
def exception_name(exc):
    return '{}.{}'.format(type(exc).__module__, type(exc).__name__)


def generate_output_writer(output_path, output_files):
    def output_writer(output_file_name, output_data, should_add_to_build):
        output_file_path = os.path.join(output_path, output_file_name)
        output_file_desc = None
        if output_file_path not in output_files:
            output_file_dir = os.path.dirname(output_file_path)
            if not os.path.isdir(output_file_dir):
                try:
                    os.makedirs(output_file_dir)
                except OSError as ex:
                    # it is possible for multiple codegen processes to attempt to create the same
                    # output directory. If the directory exists, just accept it and move on
                    if ex.errno != errno.EEXIST or not os.path.isdir(output_file_dir):
                        raise ex;
            output_file_desc = os.open(output_file_path, os.O_CREAT | os.O_TRUNC | os.O_WRONLY)
            output_files[output_file_path] = output_file_desc
        else:
            output_file_desc = output_files[output_file_path]

        os.write(output_file_desc, output_data)

        RegisterOutputFile(output_file_path, should_add_to_build)
    return output_writer


# This is the function called by the codegen utility
# @param rootPath - The path to the source script, necessary to find the modules at runtime
# @param script - The script to run, this will load the script as a python module
# @param dataObject - The data for the script to operate on, this can be JSON/XML or some custom binary format.
# @param inputPath - This is the absolute path where the inputFile is relative to
# @param outputPath - This is where the output file are required to go, but we may add folders, if necessary
# @param inputFile - This is the relative path to the input file we processed to get the dataObject
def run_scripts(scripts, data_object, input_path, output_path, input_file):
    # We want to catch any exception that happens and get some output to the user
    try:
        env = create_environment()

        # Setup the jinja2 environment
        env.jinja_env = load_jinja2_environment(scripts)

        # Track output files
        output_files = {}
        # Set the output writer
        env.output_writer = generate_output_writer(output_path, output_files)

        script_array = scripts.split(',')
        for script in script_array:
            drivers = env.get_drivers_for_script(script)
            for driver in drivers:
                driver.execute(data_object, input_file)

        # Close open file handles
        for output_file_path, output_file_desc in output_files.iteritems():
            os.close(output_file_desc)

        OutputPrint("Done")
    except BaseException as ex:
        tb_list = traceback.extract_tb(sys.exc_traceback)
        for filename, lineno, name, line in tb_list:
            OutputError('{}({}): error {}: in {}: {}'.format(filename, lineno, exception_name(ex),
                                                             name, line))
        filename, lineno, _, _ = tb_list[-1]
        OutputError('{}({}): error {}: {}'.format(filename, lineno, exception_name(ex), str(ex)))
        raise
