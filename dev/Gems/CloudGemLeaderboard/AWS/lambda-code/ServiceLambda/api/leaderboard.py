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

import service
import score_reader
from errors import ClientError


@service.api
def post(request, stat = None, additional_data = {}):
    if not stat:
        return {"scores": []}
    if additional_data == None:
        additional_data = {}
    page = additional_data.get("page", None)
    page_size = additional_data.get("page_size", None)
    has_page = False

    if not page is None and not page_size is None:
        if not isinstance(page,(int)):
            raise ClientError("Page param is not an integer")
        if not isinstance(page_size,(int)):
            raise ClientError("Page size param is not an integer")
        if page_size > 0:
            has_page = True

    leaderboard = score_reader.build_leaderboard(stat,
        additional_data.get("users", []))

    total_entries = len(leaderboard)
    if has_page:
        leaderboard = leaderboard[page * page_size : (page + 1) * page_size]

    response = {
        "scores": leaderboard
    }

    if has_page:
        response["current_page"] = page
        response["page_size"] = page_size
        if total_entries == 0:
            response["total_pages"] = 0
        elif total_entries < page_size:
            response["total_pages"] = 1
        else:
            response["total_pages"] = ((total_entries - 1) / page_size) + 1

    return response
