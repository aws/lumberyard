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
# $Revision: #5 $

import json
import os

from cgf_utils.data_utils import Data

SERVICE_DIRECTORY_PREFIX = "cloudcanvas_service_"
SERVICE_DIRECTORY_FORMAT = SERVICE_DIRECTORY_PREFIX + "{}" # keep in sync with InterfaceDependencyManager

class LambdaSettingsModule(object):

    def __init__(self):
        self.__settings = None

    def __load_settings(self):

        settings_file_path = os.path.join(os.path.dirname(__file__), 'settings.json')
        if not os.path.isfile(settings_file_path):
            print 'There is no settings file at {}.'.format(settings_file_path)
            return Data({}, read_only=True)

        with open(settings_file_path, 'r') as file:
            content = json.load(file)

        return Data(content, read_only = True)

    @property
    def settings(self):
        if self.__settings is None:
            self.__settings = self.__load_settings()
        return self.__settings

    def get_setting(self, setting_name, check_id = True):
        if setting_name in os.environ:
            print "Using the override for setting {} found in the lambda environment variables".format(setting_name)
            if not check_id:
                return os.environ[setting_name]
            settingVal = os.environ[setting_name]
            try:
                jsonSetting = json.loads(settingVal)
                return jsonSetting.get('id', settingVal)
            except:
                return settingVal
        return self.settings.DATA.get(setting_name, None)

    def get_service_url(self, service_name):
        environ_setting_name = SERVICE_DIRECTORY_FORMAT.format(service_name)
        if environ_setting_name in os.environ:
            return os.environ[environ_setting_name]
        # This is the old way of doing this. Project level interfaces could still live here
        return self.settings.DATA.get("Services", {}).get(service_name, {}).get("InterfaceUrl", "")

    def list_service_urls(self):
        out = []
        for key, val in os.environ.iteritems():
            if key.startswith(SERVICE_DIRECTORY_PREFIX):
                out.append({key[len(SERVICE_DIRECTORY_PREFIX):]: val})

        # This is the old way of doing this. Project level interfaces could still live here
        for interface, interface_data in self.settings.DATA.get("Services", {}).iteritems():
            if interface_data['InterfaceUrl']:
                out.append({interface: interface_data['InterfaceUrl']})

        return out
