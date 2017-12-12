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

import pkgutil

_settings = None

def get_setting(setting_name):
    global _settings
    if not _settings: _load_settings()
    return _settings.get(setting_name, None)

def _load_settings():
    global _settings
    loader = pkgutil.find_loader(__name__ + '.settings')
    if loader is None:
        _settings = {}
    else:
        _settings = loader.load_module(__name__ + '.settings').settings

