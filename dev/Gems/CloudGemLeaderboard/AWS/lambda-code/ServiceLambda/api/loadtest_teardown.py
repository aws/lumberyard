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

from __future__ import print_function
from errors import ClientError

import service
import score_reader
import leaderboard_constant as c


#
# Cloud Gem Leaderboard Load Test Teardown API
#
@service.api
def post(request, stat=None, additional_data={}):
    print("{} load test clean up entered.".format(c.GEM_NAME))

    if not stat:
        return {"scores": []}
    if additional_data is None:
        additional_data = {}

    page = additional_data.get("page", None)
    page_size = additional_data.get("page_size", None)
    validate_pages(page, page_size)
    has_page = (False, True)[page_size > 0]

    response = build_response(stat, additional_data, page, page_size, has_page)

    print("{} load test clean up has finished.".format(c.GEM_NAME))
    return response


#
# Check if page & page_size contain valid values
#
def validate_pages(page, page_size):
    if page is not None and page_size is not None:
        if not isinstance(page, (int)):
            raise ClientError("Page param is not an integer")
        if not isinstance(page_size, (int)):
            raise ClientError("Page size param is not an integer")


#
# Determine the number of pages and modify the response
#
def count_pages(has_page, page, page_size, response, total_entries):
    if has_page:
        response["current_page"] = page
        response["page_size"] = page_size
        if total_entries == 0:
            response["total_pages"] = 0
        elif total_entries < page_size:
            response["total_pages"] = 1
        else:
            response["total_pages"] = ((total_entries - 1) / page_size) + 1


#
# Build the leaderboard data response
#
def build_response(stat, additional_data, page, page_size, has_page):
    leaderboard = score_reader.build_leaderboard(stat, additional_data.get("users", []))
    total_entries = len(leaderboard)

    if has_page:
        leaderboard = leaderboard[page * page_size: (page + 1) * page_size]

    response = {"scores": leaderboard}

    count_pages(has_page, page, page_size, response, total_entries)

    return response
