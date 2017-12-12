#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

import service
import tts

@service.api
def post(request, request_info):
    tts.save_custom_mappings(request_info['mappings'])
    return { 'status': 'success' }

@service.api
def get(request):
    return { 'mappings': tts.get_custom_mappings() }