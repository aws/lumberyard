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
# $Revision$

import json


METADATA_VERSION_TAG = 'CustomResourceVersion'
PHYSICAL_ID_TAG = 'id'
VERSION_TAG = 'v'


class _CustomResourceInfo(object):
    def __init__(self, src_physical_id):
        self.physical_id = None
        self.create_version = None

        if src_physical_id and src_physical_id.startswith("{"):
            unpacked = json.loads(src_physical_id)
            self.physical_id = unpacked.get(PHYSICAL_ID_TAG, src_physical_id)
            self.create_version = unpacked.get(VERSION_TAG, None)
        else:
            self.physical_id = src_physical_id


def get_custom_resource_info(src_physical_id):
    return _CustomResourceInfo(src_physical_id)


def get_embedded_physical_id(src_physical_id):
    info = _CustomResourceInfo(src_physical_id)
    return info.physical_id
