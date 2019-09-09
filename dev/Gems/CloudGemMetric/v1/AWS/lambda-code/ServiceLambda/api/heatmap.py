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
import os
import util
import metric_constant as c
import json
from dynamodb import DynamoDb
from cgf_utils import custom_resource_response


# Get a specific heatmap
@service.api
def get(request, id):
    db = DynamoDb()
    result = db.get_key("heatmaps")
    if result and len(result['Items']) > 0:
        for item in result['Items']:
            for heatmap in item['value']:
                if heatmap['id'] == id:
                    return heatmap
    return None

# List existing heatmaps
@service.api
def list(request):
    db = DynamoDb()
    print "Calling list heatmaps.  This should list out the existing heatmaps"
    result = db.get_key("heatmaps")
    heatmaps = []
    if result and len(result['Items']) > 0:
        heatmaps = result['Items']
    return heatmaps

# Create a new heatmap
@service.api
def post(request, meta):
    db = DynamoDb()

    heatmapArr = []
    existingHeatmaps = db.get_key("heatmaps")
    heatmapArr = existingHeatmaps['Items'][0]['value']

    # TODO: Remove [0] if testing fails
    heatmapArr.append({
        "id": meta['id'],
        "heatmap": meta
    })
    return db.set("heatmaps", heatmapArr)

# Update an existing heatmap.
# Uses an previous heatmap ID to update the heatmap with the new meta information
@service.api
def put(request, id, meta):
    db = DynamoDb()

    existingHeatmaps = db.get_key("heatmaps")
    heatmapArr = existingHeatmaps['Items'][0]['value']

    newHeatmap = {
        "id": meta['id'],
        "heatmap": meta
    }
    heatmapArr = [newHeatmap if heatmap['id'] == id else heatmap for heatmap in heatmapArr]
    return db.set("heatmaps", heatmapArr)

# Deletes an existing heatmap
@service.api
def delete(request, id):
    db = DynamoDb()

    existingHeatmaps = db.get_key("heatmaps")
    heatmapArr = existingHeatmaps['Items'][0]['value']
    print heatmapArr

    for heatmap in heatmapArr:
        if heatmap['id'] == id:
            heatmapArr.remove(heatmap)
    print heatmapArr
    return db.set("heatmaps", heatmapArr)

def cli(context, args):
    from resource_manager_common import constant
    credentials = context.aws.load_credentials()

    resources = util.get_resources(context)
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]
    os.environ["AWS_ACCESS_KEY"] = credentials.get(
        args.profile if args.profile else context.config.user_default_profile, constant.ACCESS_KEY_OPTION)
    os.environ["AWS_SECRET_KEY"] = credentials.get(
        args.profile if args.profile else context.config.user_default_profile, constant.SECRET_KEY_OPTION)
