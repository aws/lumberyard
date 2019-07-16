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

import argparse
import boto3
import json
import os
import shutil
import sys
import tempfile
import time


class Config(object):
    def __init__(self, manifest_dict, args):
        self.manifest = manifest_dict
        self.session = boto3.Session(profile_name=args.profile) if args.profile else boto3.Session()
        self.s3_res = self.session.resource('s3')
        self.s3_client = self.session.client('s3')
        self.ec2_res = self.session.resource('ec2', region_name=args.region)  # parameterize
        self.ec2_client = self.session.client('ec2', region_name=args.region)
        self.iam_client = self.session.client('iam')
        self.key_pair_name = args.key_pair_name
        self.build_instance_type = args.instance_type
        self.volume_size = args.volume_size
        self.base_ami = args.base_ami
        self.security_group_id = args.security_group_id
        self.subnet_id = args.subnet_id
        self.role_name = args.role
        self.instance_profile_arn = None
        self.userdata = ""
        self.s3_bucket = args.s3_bucket
        self.ami_name = args.ami


def verify_unused_ami_name(config):
    print("Verifying the AMI name '{}' is unused...".format(config.ami_name))
    response = config.ec2_client.describe_images(
        Filters=[
            {
                'Name': "name",
                'Values': [config.ami_name]
            }
        ]
    )

    if len(response['Images']):
        raise RuntimeError("The AMI name '{}' is currently in use. Delete the AMI or use a different name.".format(config.ami_name))


def obtain_instance_profile_arn(config):
    print("Obtaining an instance profile for role '{}'".format(config.role_name))
    response = config.iam_client.list_instance_profiles_for_role(RoleName=config.role_name)
    profiles = response.get('InstanceProfiles', [])

    if not len(profiles):
        raise RuntimeError("No instance profiles found for role '{}'".format(config.role_name))

    config.instance_profile_arn = profiles[0]['Arn']


def upload_files(config):
    for file in config.manifest.get("files", []):
        zip_source = os.path.abspath(os.path.expandvars(file['source']))
        zip_path = tempfile.mktemp()
        zip_filename = os.path.basename(zip_path) + ".zip"
        if file['include_folder']:
            zip_root_dir, zip_base_dir = os.path.split(zip_source)
        else:
            zip_root_dir = zip_source
            zip_base_dir = None
        print("Zipping and uploading {}...".format(zip_source))

        shutil.make_archive(
            base_name=zip_path,
            format='zip',
            root_dir=zip_root_dir,
            base_dir=zip_base_dir
        )

        config.s3_res.meta.client.upload_file(
            Filename=zip_path + ".zip",
            Bucket=config.s3_bucket,
            Key=zip_filename
        )

        file['zip'] = zip_filename


def build_userdata(config):
    lines = [
        "<powershell>",
        "Add-Type -AssemblyName System.IO.Compression.FileSystem"
    ]
    for file in config.manifest.get("files", []):
        temp_zip_path = "C:\\" + file['zip']
        lines.append("Read-S3Object -BucketName \"{}\" -Key \"{}\" -File \"{}\"".format(
            config.s3_bucket,
            file['zip'],
            temp_zip_path
        ))
        lines.append("[System.IO.Compression.ZipFile]::ExtractToDirectory(\"{}\", \"{}\")".format(
            temp_zip_path,
            file['dest']
        ))

    lines.extend(config.manifest.get("commands", []))
    lines.extend([
        "C:\\ProgramData\\Amazon\\EC2-Windows\\Launch\\Scripts\\InitializeInstance.ps1 -Schedule",
        "C:\\ProgramData\\Amazon\\EC2-Windows\\Launch\\Scripts\\SysprepInstance.ps1",
        "</powershell>"
    ])
    config.userdata = "\n".join(lines)


def create_instance(config):
    print("Creating and configuring EC2 instance...")
    instance_params = {
        "BlockDeviceMappings": [
            {
                "DeviceName": "/dev/sda1",  # /dev/xvda on Linux?
                "Ebs": {
                    "VolumeSize": config.volume_size,
                    "VolumeType": "gp2"
                }
            }
        ],
        "IamInstanceProfile": {
            'Arn': config.instance_profile_arn
        },
        "ImageId": config.base_ami,
        "InstanceType": config.build_instance_type,
        "MinCount": 1,
        "MaxCount": 1,
        "UserData": config.userdata
    }

    if config.key_pair_name:
        instance_params['KeyName'] = config.key_pair_name

    if config.security_group_id:
        instance_params['SecurityGroupIds'] = [config.security_group_id]
        instance_params['SubnetId'] = config.subnet_id

    instance = config.ec2_res.create_instances(**instance_params)[0]

    print("Waiting for instance {} to initialize, configure, and shut down. This will take some time...".format(instance.id))
    previous_state = None
    while True:
        try:
            response = config.ec2_client.describe_instances(
                InstanceIds=[instance.id]
            )
        except:
            # Sometimes this fails before the instance has propagated fully
            continue

        reservations = response.get('Reservations')
        if not reservations:
            continue

        instance_info = reservations[0]['Instances'][0]
        state_name = instance_info['State']['Name']

        if state_name != previous_state:
            print("   Instance {} is now {}.".format(instance.id, state_name))
            previous_state = state_name

            if state_name == "stopped":
                break
            else:
                time.sleep(10)

    print("Building AMI from instance {}...".format(instance.id))
    response = config.ec2_client.create_image(
        InstanceId=instance.id,
        Name=config.ami_name
    )
    ami_id = response['ImageId']

    previous_state = None
    print("Waiting for AMI {} to become available...".format(ami_id))
    while True:
        response = config.ec2_client.describe_images(
            ImageIds=[ami_id]
        )

        state_name = response['Images'][0]['State']
        if state_name != previous_state:
            print("  AMI {} is now {}.".format(ami_id, state_name))
            previous_state = state_name

            if state_name == "available":
                break
            else:
                time.sleep(10)

    print("AMI {} successfully generated!".format(ami_id), "Beginning clean up.")
    print("Terminating instance {}...")
    config.ec2_client.terminate_instances(
        InstanceIds=[instance.id]
    )

    print("Deleting temporary files from S3...")
    for file in config.manifest.get("files", []):
        config.s3_client.delete_object(
            Bucket=config.s3_bucket,
            Key=file['zip']
        )

    print("Done!")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generates a Windows Server AMI with the Cloud Gem Compute Farm harness "
                                                 "installed on it, as well as any other software required by EC2 instances")

    required_group = parser.add_argument_group("required arguments")
    required_group.add_argument("--ami", help="Unique name for the AMI you want to create.", required=True)
    required_group.add_argument("--role", help="IAM role for the EC2 instance. You may use the role generated by the gem in your deployment for this.", required=True)
    required_group.add_argument("--s3-bucket", help="S3 bucket to use for file transfers. You may use the bucket generated by the gem in your deployment for this.", required=True)

    parser.add_argument("--manifest", help="Path to the manifest.json file. Defaults to ./manifest.json.", default="./manifest.json")
    parser.add_argument("--profile", help="A profile name from the .aws/credentials file to use.", default=None)
    parser.add_argument("--region", help="Name of an AWS region to use for AMI creation.", default="us-east-1")
    parser.add_argument("--key-pair-name", help="The name of your key pair. Necessary if you want to log into your instance for debugging.", default=None)
    parser.add_argument("--instance-type", help="EC2 instance type to use for building your AMI. (Does not have to be the same type used for deploying.)", default="m4.large")
    parser.add_argument("--volume-size", help="Volume size for your AMI in GiB. Deployed instances will be required to be at least this size.", type=int, default=100)
    parser.add_argument("--base-ami", help="Base AMI to use as starting point for building custom AMI.", default="ami-0a792a70")
    parser.add_argument("--subnet-id", help="An optional subnet ID to use for the staging EC2 instance. Must also specify --security-group-id if used.", default=None)
    parser.add_argument("--security-group-id", help="An optional security group ID to use for the staging EC2 instance. Must also specify --subnet-id if used.", default=None)

    args = parser.parse_args()

    if (args.subnet_id is None) != (args.security_group_id is None):
        raise RuntimeError("Parameters --subnet-id and --security-group-id must be specified together if used")

    with open(args.manifest, "r") as fp:
        manifest = json.load(fp)

    ami_config = Config(manifest, args)

    verify_unused_ami_name(ami_config)
    obtain_instance_profile_arn(ami_config)
    upload_files(ami_config)
    build_userdata(ami_config)
    create_instance(ami_config)
