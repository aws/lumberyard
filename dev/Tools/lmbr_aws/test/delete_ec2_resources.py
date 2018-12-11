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
# $Revision: #26 $

import argparse
import time
import sys

import cleanup_utils


def wrapped_delete(wrapped_function):
    def wrapper(cleaner):
        base_name = wrapped_function.__name__[len("_delete_"):]
        skip_name = "skip_" + base_name
        if not getattr(cleaner.args, skip_name):
            type_name = base_name.replace("_", " ")
            print("Deleting {}...".format(type_name))
            count = wrapped_function(cleaner)
            print("Deleted {} {}.".format(count, type_name))
        else:
            print("Skipping {}.".format(base_name.replace("_", " ")))

    return wrapper


class Cleaner:
    def __init__(self, session, args):
        self.args = args
        self.ec2 = session.client('ec2')
        self.autoscaling = session.client('autoscaling')
        self.security_groups = []
        self.security_groups_by_id = {}

    def cleanup(self):
        self._delete_auto_scaling_groups()
        self._delete_ec2_instances()
        self._delete_auto_scaling_configs()
        self._delete_amis()

        self._wait_for_ec2_instances_to_terminate()
        self._delete_security_groups()
        self._delete_route_tables()
        self._delete_subnets()
        self._delete_internet_gateways()
        self._delete_nat_gateways()
        self._delete_network_acls()
        self._delete_vpcs()
        self._delete_dhcp_options()

    @wrapped_delete
    def _delete_auto_scaling_groups(self):
        return self.__delete_items(
            self.autoscaling.describe_auto_scaling_groups, self.autoscaling.delete_auto_scaling_group,
            'AutoScalingGroupName', 'AutoScalingGroups')

    @wrapped_delete
    def _delete_ec2_instances(self):
        params = {
            'Filters': [
                {'Name': 'instance-state-name', 'Values': ['pending', 'running']}
            ]
        }
        paginator = cleanup_utils.nt_paginate(self.ec2.describe_instances, params)
        instance_ids = [x['InstanceId'] for result in paginator for reserve in result['Reservations'] for x in
                        reserve['Instances']]
        if len(instance_ids):
            for instance_id in instance_ids:
                self.__log_item(instance_id)
            self.ec2.terminate_instances(InstanceIds=instance_ids)
        return len(instance_ids)

    @wrapped_delete
    def _delete_auto_scaling_configs(self):
        return self.__delete_items(
            self.autoscaling.describe_launch_configurations, self.autoscaling.delete_launch_configuration,
            'LaunchConfigurationName', 'LaunchConfigurations')

    @wrapped_delete
    def _delete_amis(self):
        return self.__delete_items(self.ec2.describe_images, self.ec2.deregister_image, 'ImageId', 'Images',
                                   params={'Owners': ['self'], 'Filters': [{'Name': "state", 'Values': ["available"]}]})

    def _wait_for_ec2_instances_to_terminate(self):
        print("Waiting for all EC2 instances to terminate before proceeding...")
        while True:
            result = self.ec2.describe_instances(Filters=[
                {'Name': 'instance-state-name', 'Values': ['pending', 'running', 'shutting-down']}
            ])
            instance_ids = [x['InstanceId'] for reserve in result['Reservations'] for x in result['Instances']]
            count = len(instance_ids)
            display_count = count if len(result.get('NextToken', "")) == 0 else "more than {}".format(count)
            print("    Instances remaining: {}".format(display_count))
            if count > 0:
                time.sleep(2)
            else:
                break

    @wrapped_delete
    def _delete_security_groups(self):
        return self.__delete_items(
            self.ec2.describe_security_groups, self.ec2.delete_security_group, 'GroupId', 'SecurityGroups',
            condition=lambda x: x['GroupName'] != "default")

    @wrapped_delete
    def _delete_route_tables(self):
        def cleanup_route_table(route_table):
            associations = [association for association in route_table['Associations']]
            is_main_table_for_vpc = False
            for association in associations:
                if association['Main']:
                    is_main_table_for_vpc = True
                else:
                    association_id = association['RouteTableAssociationId']
                    self.__log_item("    Disassociating AssociationId {}".format(association_id))
                    self.ec2.disassociate_route_table(AssociationId=association_id)
            return not is_main_table_for_vpc

        return self.__delete_items(self.ec2.describe_route_tables, self.ec2.delete_route_table, 'RouteTableId',
                                   'RouteTables', cleanup=cleanup_route_table)

    @wrapped_delete
    def _delete_subnets(self):
        return self.__delete_items(self.ec2.describe_subnets, self.ec2.delete_subnet, 'SubnetId', 'Subnets')

    @wrapped_delete
    def _delete_internet_gateways(self):
        def cleanup_attachments(ig):
            attachments = [(ig['InternetGatewayId'], attachment['VpcId']) for attachment in ig['Attachments']]
            for gateway_id, vpc_id in attachments:
                params = {'InternetGatewayId': gateway_id, 'VpcId': vpc_id}
                self.__log_item("    Detaching {}".format(params))
                self.ec2.detach_internet_gateway(**params)
            return True

        return self.__delete_items(self.ec2.describe_internet_gateways, self.ec2.delete_internet_gateway,
                                   'InternetGatewayId', 'InternetGateways', cleanup=cleanup_attachments)

    @wrapped_delete
    def _delete_nat_gateways(self):
        return self.__delete_items(self.ec2.describe_nat_gateways, self.ec2.delete_nat_gateway, 'NatGatewayId',
                                   'NatGateways')

    @wrapped_delete
    def _delete_network_acls(self):
        return self.__delete_items(self.ec2.describe_network_acls, self.ec2.delete_network_acl,
                                   'NetworkAclId', 'NetworkAcls', condition=lambda x: not x['IsDefault'])

    @wrapped_delete
    def _delete_dhcp_options(self):
        return self.__delete_items(self.ec2.describe_dhcp_options, self.ec2.delete_dhcp_options,
                                   'DhcpOptionsId', 'DhcpOptions')

    @wrapped_delete
    def _delete_vpcs(self):
        return self.__delete_items(self.ec2.describe_vpcs, self.ec2.delete_vpc, 'VpcId', 'Vpcs')

    def __log_item(self, item_name):
        print("    {}".format(item_name))

    def __delete_items(self, list_fn, delete_fn, item_key, group_key, params=None, condition=lambda x: True,
                       cleanup=lambda x: True):
        delete_param_key = item_key
        paginator = cleanup_utils.nt_paginate(list_fn, params if params else {})
        items = [x for result in paginator for x in result[group_key] if condition(x)]
        count = 0
        for item in items:
            item_id = item[item_key]
            self.__log_item(item_id)
            if cleanup(item):
                params = {delete_param_key: item_id}
                delete_fn(**params)
                count += 1
        return count


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Deletes all EC2- and VPC-related resources in an AWS account. "
                                     "Deletes ALL resources in the region regardless of name, so use with caution!")
    parser.add_argument('--confirm-delete-all', required=False, action='store_true', default=False,
                        help="Required to confirm the deletion operation.")
    cleanup_utils.add_common_arguments(parser)

    # Add "--skip-x" options for each of the delete methods in our cleaner
    for function_name in dir(Cleaner):
        if function_name.startswith("_delete_") and callable(getattr(Cleaner, function_name)):
            base_name = function_name[len("_delete_"):]
            skip_name = "--skip-{}".format(base_name.replace("_", "-"))
            help = "Skip deleting {}".format(base_name.replace("_", " "))
            parser.add_argument(skip_name, required=False, action='store_true', default=False, help=help)

    args = parser.parse_args()

    if not args.confirm_delete_all:
        print("You must supply --confirm-delete-all to accept responsibility for deleting all EC2/VPC resources.")
        sys.exit(1)

    session = cleanup_utils.get_session(args)
    cleaner = Cleaner(session, args)
    cleaner.cleanup()
