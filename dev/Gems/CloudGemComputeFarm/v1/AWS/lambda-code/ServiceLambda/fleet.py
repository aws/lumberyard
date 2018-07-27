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
import json
import boto3
import CloudCanvas

from botocore.exceptions import ClientError

FLEET_CONFIGURATION_KEY = 'fleet_configuration.json'
SUCCEED_STATUS = 'SUCCEED'

INSTANCE_STARTUP_SCRIPT = """<script>
start cmd /c C:\Python36\python.exe C:\CloudCanvasHarness\main.py --domain %(domain)s --task-list %(task_list)s --div-task %(div_task)s --merge-task %(merge_task)s --build-task %(build_task)s --config-bucket %(s3)s --log-db %(log_db)s --stdout C:\worker.log
start cmd /c C:\Python36\python.exe C:\CloudCanvasHarness\main.py --domain %(domain)s --task-list %(task_list)s --div-task %(div_task)s --merge-task %(merge_task)s --build-task %(build_task)s --config-bucket %(s3)s --log-db %(log_db)s -rd --stdout C:\decider.log
</script>
<persist>true</persist>
"""


def post_fleet_config(config):
    bucket = boto3.resource('s3').Bucket(CloudCanvas.get_setting("computefarm"))

    try:
        bucket.put_object(Body=json.dumps(config), Key=FLEET_CONFIGURATION_KEY)
    except ClientError as e:
        return e.response['Error']['Code']

    return SUCCEED_STATUS

def get_fleet_config():
    client = boto3.client('s3')

    try:       
        response = client.get_object(Bucket=CloudCanvas.get_setting("computefarm"), Key=FLEET_CONFIGURATION_KEY)
        body = json.loads(response["Body"].read())
    except ClientError as e:
        if e.response['Error']['Code'] == 'AccessDenied':
            return {}
    
    if 'instanceNum' in body:
        # Attempt to load the current number of instances from the autoscaling group; leave it as-is if unavailable
        try:
            response = __get_autoscaling_client().describe_auto_scaling_groups(AutoScalingGroupNames=[body['autoScalingGroupName']])
            body['instanceNum'] = response['AutoScalingGroups'][0]['DesiredCapacity']
        except:
            pass

    return body

def get_amis():
    client = boto3.client('ec2')
    response = client.describe_images(Owners=['self'])
    result = response['Images']
    result.sort(key=lambda x: x['Name'])
    
    return result
    
def get_key_pairs():
    client = boto3.client('ec2')
    response = client.describe_key_pairs()
    result = [item['KeyName'] for item in response['KeyPairs']]
    
    return result

def create_launch_configuration(config):
    startup_script = INSTANCE_STARTUP_SCRIPT % {
        'domain': CloudCanvas.get_setting('DomainName'),
        'task_list': CloudCanvas.get_setting('TaskList'),
        'div_task': CloudCanvas.get_setting('DivTask'),
        'build_task': CloudCanvas.get_setting('BuildTask'),
        'merge_task': CloudCanvas.get_setting('MergeTask'),
        'log_db': CloudCanvas.get_setting('LogDB'),
        's3': CloudCanvas.get_setting('S3Bucket')
    }

    try:
        params = {
            'IamInstanceProfile': CloudCanvas.get_setting("InstanceProfile"),
            'LaunchConfigurationName': config['launchConfigurationName'],
            'ImageId': config['imageId'],
            'InstanceType': config['instanceType'],
            'SecurityGroups': [CloudCanvas.get_setting("EC2SecurityGroup")],
            'InstanceMonitoring': {'Enabled': False},
            'AssociatePublicIpAddress': True,
            'UserData': startup_script
        }
        
        if len(config['keyPair']):
            params['KeyName'] = config['keyPair']
        
        __get_autoscaling_client().create_launch_configuration(**params)
        
    except ClientError as e:
        return e.response['Error']['Code']

    return SUCCEED_STATUS

def create_or_update_autoscaling_group(config):
    create_autoscaling_group = False if get_fleet_config() else True

    try:
        if create_autoscaling_group:
            __get_autoscaling_client().create_auto_scaling_group(
                AutoScalingGroupName = config['autoScalingGroupName'],
                LaunchConfigurationName = config['launchConfigurationName'],
                MinSize = config['minSize'],
                MaxSize = config['maxSize'],
                VPCZoneIdentifier = ",".join([CloudCanvas.get_setting("EC2Subnet" + x) for x in ("A", "B", "C")])
                )
        else:
            __get_autoscaling_client().update_auto_scaling_group(
                AutoScalingGroupName = config['autoScalingGroupName'],
                LaunchConfigurationName = config['launchConfigurationName'],
                MinSize = config['minSize'],
                MaxSize = config['maxSize'],
                VPCZoneIdentifier = ",".join([CloudCanvas.get_setting("EC2Subnet" + x) for x in ("A", "B", "C")])
                )
    except ClientError as e:
        return e.response['Error']['Code']

    return SUCCEED_STATUS

def delete_autoscaling_group(name):
    try:
        __get_autoscaling_client().delete_auto_scaling_group(
            AutoScalingGroupName=name,
            ForceDelete=True
            )

        client = boto3.client('s3')
        client.delete_object(Bucket=CloudCanvas.get_setting("computefarm"), Key=FLEET_CONFIGURATION_KEY)
    except ClientError as e:
        return e.response['Error']['Code']

    return SUCCEED_STATUS

def delete_launch_configuration(name):
    try:
        response = __get_autoscaling_client().delete_launch_configuration(
            LaunchConfigurationName=name
            )
    except ClientError as e:
        return e.response['Error']['Code']

    return SUCCEED_STATUS

def __get_autoscaling_client():
    if not hasattr(__get_autoscaling_client,'autoscaling_group'):
        __get_autoscaling_client.autoscaling_group = boto3.client('autoscaling')
    return __get_autoscaling_client.autoscaling_group