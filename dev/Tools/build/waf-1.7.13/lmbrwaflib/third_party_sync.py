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

from waflib import Utils
from waflib.Configure import conf, Logs

import os
import ConfigParser
import subprocess


# Config file values for the p4 sync configuration file
P4_SYNC_CONFIG_SECTION = "3rdParty"         # The config section where the keys are stored under
P4_SYNC_CONFIG_EXECUTABLE = "p4_exe"        # The value of the p4 executable (full path) on the local machine
P4_SYNC_CONFIG_HOST_PORT = "p4_port"        # The host and port of the perforce server, ie  perforce.com:1666
P4_SYNC_CONFIG_WORKSPACE = "p4_workspace"   # The client workspace that has the 3rd party root mapped
P4_SYNC_CONFIG_REPOSITORY = "p4_repo"       # The location of the repo in the perforce host that references the root of the 3rd party folder


class P4SyncThirdPartySettings:
    """
    This class manages a p4 settings that will attempt to sync any missing local 3rd party libraries from a perforce
    repository
    """

    def __init__(self, local_3rd_party_root, config_file):
        """
        Initialize the sync settings object to a config file.  This will also attempt to connect to the perforce host
        via the settings if any to determine the local workspace root as well.  If all goes well, then the object
        will be marked as valid (see is_valid())
        """
        self.valid = False

        if not os.path.exists(config_file):
            return

        try:
            config_parser = ConfigParser.ConfigParser()
            config_parser.read(config_file)

            self.p4_exe = config_parser.get(P4_SYNC_CONFIG_SECTION, P4_SYNC_CONFIG_EXECUTABLE)
            self.p4_port = config_parser.get(P4_SYNC_CONFIG_SECTION, P4_SYNC_CONFIG_HOST_PORT)
            self.p4_workspace = config_parser.get(P4_SYNC_CONFIG_SECTION, P4_SYNC_CONFIG_WORKSPACE)
            self.p4_repo = config_parser.get(P4_SYNC_CONFIG_SECTION, P4_SYNC_CONFIG_REPOSITORY)
        except ConfigParser.Error as err:
            Logs.warn('[WARN] Error reading p4 sync settings file {}.  ({}).  '
                      'Missing 3rd party libraries will not be synced from perforce'.format(config_file, str(err)))
            return

        try:
            proc = subprocess.Popen([self.p4_exe, '-p', self.p4_port, 'client', '-o', self.p4_workspace],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE)
            for line in iter(proc.stdout.readline, b''):
                if line.startswith("Root:"):
                    self.client_root = line.replace("Root:", "").strip()
                    break
        except OSError as err:
            Logs.warn('[WARN] Invalid values in the p4 sync settings file {}.  ({}).  '
                      'Missing 3rd party libraries will not be synced from perforce'.format(config_file, str(err)))
            return

        normalized_client_root = os.path.normcase(self.client_root)
        normalized_3rd_party_root = os.path.normcase(local_3rd_party_root)
        if not normalized_3rd_party_root.startswith(normalized_client_root):
            Logs.warn('[WARN] Local 3rd Party root ({}) does not match the root from workspace "{}" ({})'
                      'Missing 3rd party libraries will not be synced from perforce'.format(local_3rd_party_root,
                                                                                            self.p4_workspace,
                                                                                            self.client_root))
            return
        self.valid = True

    def is_valid(self):
        return self.valid

    def sync_3rd_party(self, third_party_subpath):

        Logs.info("[INFO] Syncing library {} from perforce...".format(third_party_subpath))
        try:
            timer = Utils.Timer()

            sync_proc = subprocess.Popen([self.p4_exe,
                                          '-p', self.p4_port,
                                          '-c', self.p4_workspace,
                                          'sync',
                                          '-f', "{}/{}/...".format(self.p4_repo, third_party_subpath.rstrip('/'))],
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)
            sync_out, sync_err = sync_proc.communicate()
            if not sync_err:
                Logs.info("[INFO] Library {} synced.  ({})".format(third_party_subpath, str(timer)))

        except OSError as err:
            Logs.warn("[WARN] Unable to sync 3rd party path {}: {}".format(third_party_subpath, str(err)))


