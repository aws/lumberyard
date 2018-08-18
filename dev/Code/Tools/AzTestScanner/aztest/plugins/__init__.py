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
Registers subparser plugins to the AzTest argparse.ArgumentParser, exposing custom commands.

To hook a new command and subparser, use the following pattern:
    def my_command(known_args, extra_args):
        <implementation>

    def aztest_add_subparsers(subparsers):
        command = subparsers.add_parser("commandline_key")
        <argparse configuration such as add_argument>
        command.set_defaults(func=my_command)

Subparser Initialization order:
    1. Plugin parsers with the 'def aztest_add_subparsers(subparsers):' function hook
        a. Modules directly in /aztest/plugins/
        b. Child submodules in /aztest/plugins/<module> with the hook in their __init__.py
            (grandchild modules will not be scanned)
    2. AzTest parsers

* Note that the last module defining a command wins, as argparse silently replaces duplicate parsers options. Due to
    the iteration order of the module namespace being OS-dependent, avoiding duplication is highly recommended
"""

import importlib
import inspect
import pkgutil


def _iter_namespace(ns_pkg):
    return pkgutil.iter_modules(ns_pkg.__path__, ns_pkg.__name__ + ".")


def _get_plugins():
    current_module = inspect.getmodule(inspect.currentframe())
    aztest_plugins = {
        name: importlib.import_module(name)
        for finder, name, ispkg
        in _iter_namespace(current_module)
    }
    return aztest_plugins


def subparser_hook(subparsers):
    """
    Visits aztest_add_subparsers(subparsers) on all matching modules in the current module's directory, allowing them
        to register new argument subparsers
    :param subparsers: Target subparsers object from ArgumentParser
    """
    plugins = _get_plugins()
    for name in plugins:
        plugin = plugins[name]
        for function_entry in inspect.getmembers(plugin, inspect.isfunction):
            function_name = function_entry[0]
            if function_name == "aztest_add_subparsers":
                plugin.aztest_add_subparsers(subparsers)
