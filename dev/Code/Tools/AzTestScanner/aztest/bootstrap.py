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
import os.path


class Bootstrapper(object):
    """executable used to load and initialize an environment for a module under test

    Bootstrapper is initialized from json data, and can be queried for the command
    line required to start the executable to use as a bootstrapper. Caller should
    use the command line to invoke a subprocess call.
    """
    def __init__(self, json_config):
        """ initialize the bootstrapper info

        If the name of the module is 'DEFAULT', this Bootstrapper will be used
        as the default fallback for modules that are not configured.

        :param json_config: data loaded via the json package
        """
        self._config = json_config
        self._is_default = json_config['module'] == 'DEFAULT'

    @property
    def command_line(self):
        """ generate a tuple to be used with subprocess call to invoke the bootstrapping executable
        :return: (exe, args...) to be used with subprocess invoke
        :rtype: tuple
        """
        args = [
            self._config['bootstrapper']
        ]

        # process arguments and replace any $ variables with values from the config
        config_args = self._config['args'] if 'args' in self._config else []
        xargs = (self._config[x[1:]] if x[0] == '$' else x for x in config_args)
        args += xargs

        # append module to end if $module is not present in args
        if '$module' not in config_args:
            args += [self._config['module']]

        return tuple(args)

    @property
    def is_default(self):
        """
        :return: true, if this is the default fallback bootstrapper
        """
        return self._is_default

    @property
    def module(self):
        """
        :return: name of the module (e.g. CryDesigner.dll)
        :rtype: str
        """
        return self._config['module']


class BootstrapConfig(object):
    """ bootstrapper configuration data for all modules

    Load the full bootstrapper configuration data from json data, and provide convenient
    lookups to find the bootstrapper to use with any module. Returns a default bootstrapper
    if one is specified in the config file, when no config entry matches the module name. Optionally
    flattens the paths when searching for modules, so that '/Foo/Bar.dll' can be found by querying
    for 'Bar.dll'.
    """

    def __init__(self, flatten=False):
        """
        :param bool flatten: if true, stores and looks up modules without path information
        """
        self._modules = {}
        self._default_cfg = None
        self._flatten = flatten

    def load(self, json_config):
        """ load the entire bootstrapper config for all modules from pre-loaded json data
        :param json_config: json data configuration for all modules
        """
        for config in json_config['modules']:
            module = config['module']
            if module == 'DEFAULT':
                self._default_cfg = config
            else:
                if (self._flatten):
                    module = os.path.split(module)[1]
                self._modules[module] = config

    def get_bootstrapper(self, module):
        """ lookup the bootstrapper to use for the module

        :param str module: relative path to module, or simply the module name, e.g. "EditorPlugins/CryDesigner.dll"
        :return: a Bootstrapper instance, which may be the default bootstrapper, or None if there was a configuration
            error
        """
        if self._flatten:
            module = os.path.split(module)[1]

        cfg = self._modules.get(module, None)
        if cfg:
            if "bootstrapper" not in cfg:
                return None
            else:
                return Bootstrapper(cfg)

        return Bootstrapper(self._default_cfg) if self._default_cfg else None
