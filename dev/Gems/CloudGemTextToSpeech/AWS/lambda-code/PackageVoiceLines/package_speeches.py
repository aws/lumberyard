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

import zip_exporter
import boto3
import json
import CloudCanvas

def main(json_input, context):
    tts_info_list = json_input['tts_info_list']
    zip_exporter.create_zip_file(tts_info_list, json_input['name'], json_input['UUID'])