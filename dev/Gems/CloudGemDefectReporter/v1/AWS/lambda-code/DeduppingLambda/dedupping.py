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

import jira_integration

def main(report, context):
    issue_id = ''

    # Implement you custom dedupping algorithm here
    # The sample code below quries the JIRA database and return at most 50 issues

    # Search for issues using the JIRA Query Language (JQL): https://developer.atlassian.com/cloud/jira/platform/rest/#api/2/search-search
    # Advanced searching using the JIRA Query Language (JQL): https://confluence.atlassian.com/jiracoreserver073/advanced-searching-861257209.html

    #jira_client = jira_integration.get_jira_client()

    #try:
    #    issues = jira_client.search_issues('', startAt = 0, maxResults = 50, 
    #            validate_query = True, fields = None, expand = None, json_result = None)
    #except JIRAError as e:
    #    print e.text

    return issue_id