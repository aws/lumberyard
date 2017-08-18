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
import unittest
import json
from aztest.bootstrap import BootstrapConfig


class BootstrapTest(unittest.TestCase):
    SIMPLE_CONFIG = """
    {
        "modules" : [
            {
                "module" : "DEFAULT",
                "bootstrapper" : "AzTestRunner.exe",
                "args" : []
            },
            {
                "module" : "foo.dll",
                "bootstrapper" : "foo.exe",
                "args" : ["--unittest", "$module"]
            },
            {
                "module" : "bar.dll",
                "bootstrapper" : "bar.exe",
                "args" : ["$x"],
                "x" : "x_sub"
            },
            {
                "module" : "missing_all_props.dll"
            },
            {
                "module" : "missing_args.dll",
                "bootstrapper" : "bs.exe"
            },
            {
                "module" : "no_module_var.dll",
                "bootstrapper" : "bs.exe",
                "args" : ["1", "2"]
            },
            {
                "module" : "subdir/nested.dll",
                "bootstrapper" : "bs.exe",
                "args" : []
            }
        ]
    }
    """

    def setUp(self):
        self._config = BootstrapConfig()
        self._config.load(json.loads(BootstrapTest.SIMPLE_CONFIG))
        pass

    def reconfigWithFlattening(self):
        """ override setUp configuration with a bootstrapper which flattens the lookup of modules
        """
        self._config = BootstrapConfig(flatten=True)
        self._config.load(json.loads(BootstrapTest.SIMPLE_CONFIG))

    def test_getBootstrapper_notInConfig_returnsNonNullDefault(self):
        bs = self._config.get_bootstrapper("no_bootstrapper.dll")
        self.assertTrue(bs.is_default)

    def test_getBootstrapper_inConfig_returnsIsNotDefault(self):
        bs = self._config.get_bootstrapper("foo.dll")
        self.assertFalse(bs.is_default)

    def test_getBootstrappeer_inConfig_returnIsNotNone(self):
        bs = self._config.get_bootstrapper("foo.dll")
        self.assertIsNotNone(bs)

    def test_getBootstrapper_subdirInConfigRequestIsFlatNoFlatten_returnsDefault(self):
        bs = self._config.get_bootstrapper("nested.dll")
        self.assertTrue(bs.is_default)

    def test_getBootstrapper_subdirInConfigAndRequestNoFlatten_found(self):
        bs = self._config.get_bootstrapper("subdir/nested.dll")
        self.assertEqual(bs.module, 'subdir/nested.dll')

    def test_getBootstrapper_subdirInConfigFlatRequestFlatten_found(self):
        self.reconfigWithFlattening()
        bs = self._config.get_bootstrapper("nested.dll")
        self.assertEqual(bs.module, 'subdir/nested.dll')

    def test_getBootstrapper_subdirInConfigAndRequestFlattened_found(self):
        self.reconfigWithFlattening()
        bs = self._config.get_bootstrapper("subdir/nested.dll")
        self.assertEqual(bs.module, 'subdir/nested.dll')

    def test_getCommandLine_withSubstitution_doesArgSubstitution(self):
        cmd = self._config.get_bootstrapper("foo.dll").command_line
        self.assertEqual(('foo.exe', '--unittest', 'foo.dll'), cmd)

    def test_getCommandLine_arbitrarySubstitution_doesArgSubstitution(self):
        cmd = self._config.get_bootstrapper("bar.dll").command_line
        self.assertEqual(('bar.exe', 'x_sub', 'bar.dll'), cmd)

    def test_getBootstrapper_missingAllProperties_returnsNone(self):
        cmd = self._config.get_bootstrapper("missing_all_props.dll")
        self.assertIsNone(cmd)

    def test_getCommandLine_missingArgs_returnsModuleAfterBootstrapper(self):
        cmd = self._config.get_bootstrapper("missing_args.dll").command_line
        self.assertEqual(('bs.exe', 'missing_args.dll'), cmd)

    def test_getCommandLine_argsDontContainModuleVariable_moduleAppendedToEnd(self):
        cmd = self._config.get_bootstrapper("no_module_var.dll").command_line
        self.assertEqual(('bs.exe', '1', '2', 'no_module_var.dll'), cmd)
