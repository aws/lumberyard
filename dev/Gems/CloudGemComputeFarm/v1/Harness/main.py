import argparse
import boto3
import json
import os
import sys
import urllib
import urllib.request
import uuid
import traceback
from botocore.exceptions import ClientError

from dictionary_sorter import divide
from dictionary_sorter import merge
from dictionary_sorter import build

from harness import config
from harness import decider
from harness import worker
from harness import cloudwatch

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--domain", required=True)
    parser.add_argument("-t", "--task-list", required=True)
    parser.add_argument("--div-task", required=True)
    parser.add_argument("--div-task-version", default="1.0")
    parser.add_argument("--merge-task", required=True)
    parser.add_argument("--merge-task-version", default="1.0")
    parser.add_argument("--build-task", default=None)
    parser.add_argument("--build-task-version", default="1.0")
    parser.add_argument("-rd", "--run-decider", action="store_true")
    parser.add_argument("--region", default=None)
    parser.add_argument("--config-bucket", default=None)
    parser.add_argument("--log-group", default=None)
    parser.add_argument("--log-db", default=None)
    parser.add_argument("--kvs-db", default=None)
    parser.add_argument("--profile", default=None)
    parser.add_argument("--role-arn", default=None)
    parser.add_argument("--stdout", default=None)
    args = parser.parse_args()


    try:
        # Fetch instance identity: https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/instance-identity-documents.html
        with urllib.request.urlopen('http://169.254.169.254/latest/dynamic/instance-identity/document') as response:
            info = json.load(response)
            ec2_region = info['region']
            identity = info['instanceId']
            print("Running on EC2 instance {} in region {}".format(identity, ec2_region))
    except:
        ec2_region = "us-east-1"
        identity = os.environ.get("COMPUTERNAME", "<unavailable>")
        print("Couldn't load EC2 instance data from environment, using computer hostname {}".format(identity))

    if not args.region:
        args.region = ec2_region

    # You can supply a profile to use if you are testing locally.
    session = boto3.Session(profile_name=args.profile)

    # You can supply a role arn to use if you are testing locally.
    if args.role_arn:
        sts_result = session.client('sts').assume_role(
            DurationSeconds=3600,
            RoleSessionName="Harness-" + str(uuid.uuid4()),
            RoleArn=args.role_arn
        )['Credentials']
        session = boto3.Session(
            aws_access_key_id=sts_result['AccessKeyId'],
            aws_secret_access_key=sts_result['SecretAccessKey'],
            aws_session_token=sts_result['SessionToken']
        )

    if args.stdout:
        if args.stdout == 'cloudwatch':
            writeHandler = cloudwatch.OutputHandler('HARNESS-DEBUG', session, args.region, identity, 'decider' if args.run_decider else 'worker')
        else:
            fp = open(args.stdout, "w")
            sys.stdout = fp
            sys.stderr = fp

    divide_task = config.TaskConfig(args.div_task, args.div_task_version, divide.handler)
    merge_task = config.TaskConfig(args.merge_task, args.merge_task_version, merge.handler)
    build_task = config.TaskConfig(args.build_task, args.build_task_version, build.handler) if args.build_task else merge_task
    harness_config = config.Config(session, args.region, args.domain, args.task_list, divide_task, build_task,
                                   merge_task, args.log_group, args.log_db, args.kvs_db, args.config_bucket, identity)

    try:
        if args.run_decider:
            decider.run_decider(harness_config)
        else:
            worker.run_worker(harness_config)
    except Exception as e:
            message = "Error - " + str(e) + "\n" + traceback.format_exc()
            print(message)
