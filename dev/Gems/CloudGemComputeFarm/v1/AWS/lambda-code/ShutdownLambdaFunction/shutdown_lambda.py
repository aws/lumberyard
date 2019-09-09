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


import traceback
import json
import os
import boto3

import fleet
import fleet_status

def get_autoscale_client():
    if not hasattr(get_autoscale_client, 'client'):
        get_autoscale_client.client = boto3.client('autoscaling', region_name=os.environ['AWS_DEFAULT_REGION'])
    return get_autoscale_client.client

def shutdown_autoscale_group(group_name):
    autoscale_client = get_autoscale_client()
    autoscale_response = autoscale_client.update_auto_scaling_group(
        AutoScalingGroupName=group_name,
        MinSize=0,
        DesiredCapacity=0
    )
    print('Got shutdown response {}'.format(autoscale_response))


def handler(event, context):
    result = fleet.get_fleet_config()
    cur_instances = result.get('instanceNum',0)

    if not cur_instances:
        print('No active instances, returning')
        return

    shutdown_minutes = result.get('inactivityShutdown')
    if not shutdown_minutes:
        print('No shutdown timer set, returning')
        return

    minute_diff = fleet_status.get_minutes_since_activity(result.get('updateTime'))
    print('Got time difference of {} minutes, shutdown timer is {}'.format(minute_diff, shutdown_minutes))

    if minute_diff >= shutdown_minutes:
        autoscale_group_name = result.get('autoScalingGroupName', None)
        print('Shutting down {}'.format(autoscale_group_name))
        shutdown_autoscale_group(autoscale_group_name)





