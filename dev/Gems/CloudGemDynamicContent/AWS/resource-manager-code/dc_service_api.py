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
# Provides replacement functionality for DynamicContent's CGP functions for working with
# uploaded content
import boto3

import content_bucket
import dynamic_content_settings
import resource_manager.util
import staging

from resource_manager.service import Service
from resource_manager.service import ServiceLogs

class HierarchyNode:
    """
    Simple tree data structure to hold data about uploaded content
    """
    _ignored_fields = {'FileName', 'StagingStatus', 'Parent'}

    def __init__(self, data):
        self.children = []
        self.data = data

    @staticmethod
    def pretty_print(node):
        """
        Pretty prints the data contained in a Hierarchy Node.
        * Handles dict values being inserted out of order.
        * Does not show parent data as its displayed when hierarchy is printed
        """
        if not node.data:
            return {}
        else:
            prefix = f"{node.data['FileName']} {node.data['StagingStatus']} "
            copy_data = dict(node.data)
            for key in HierarchyNode._ignored_fields:
                if key in copy_data:
                    copy_data.pop(key)
            prefix += str(copy_data)
        return prefix


def _generate_hierarchy(contents: dict) -> {str: HierarchyNode}:
    """Takes a PortalFileInfoList part of the response of Get on ServiceAPI for portal/content
    and generates a hierarchy of PAK files and manifests as a set of filename -> HierarchyNodes.

    Aims to provide similar view from top level content to their children, as seen in CloudGemPortal"""
    seen_content = {}

    for content in contents:
        file_name = content['FileName']

        if file_name in seen_content:
            # Have seen this node before as a parent link, so set the data
            entry = seen_content[file_name]
            entry.data = content
        else:
            # New entry
            entry = HierarchyNode(content)

        if 'Parent' in content:
            # If entry has a parent node, build child relationship
            parent = content['Parent']
            if parent not in seen_content:
                parent_entry = HierarchyNode(None)
                seen_content[parent] = parent_entry
            else:
                parent_entry = seen_content[parent]
            parent_entry.children.append(entry)
        else:
            # Content is not part of hierarchy so add to root level
            seen_content[file_name] = entry
    return seen_content


def _display_hierarchy(node: HierarchyNode, depth: int) -> None:
    """
    Util to aid pretty print a collection of HierarchyNodes
    """
    print(f"{'--->' * depth}{HierarchyNode.pretty_print(node)}")
    depth += 1
    for child in node.children:
        _display_hierarchy(child, depth)


def _show_hierarchy(hierarchy: {str: HierarchyNode}) -> None:
    """
    Pretty print a collection of HierarchyNodes
    """
    for entry in hierarchy:
        print('\n')
        _display_hierarchy(hierarchy[entry], depth=0)


def __get_service_stack_id(context, args) -> str:
    deployment_name = args.deployment_name or None

    resource_group_name = dynamic_content_settings.get_default_resource_group()
    if deployment_name is None:
        deployment_name = context.config.default_deployment

    stack_id = context.config.get_resource_group_stack_id(deployment_name, resource_group_name, optional=True)
    if not stack_id:
        raise RuntimeError('Unable find to deployment stack for DynamicContent')
    return stack_id


def _get_service_api_client(context, args):
    """
    Get a service client based on the DynamicContent deployment stack
    """
    stack_id = __get_service_stack_id(context, args)
    service = Service(stack_id=stack_id, context=context)
    resource_region = resource_manager.util.get_region_from_arn(stack_id)
    session = boto3.Session(region_name=resource_region)
    return service.get_service_client(session)


def _service_call_list_content(context, args) -> None:
    """Call the ServiceAPI to list uploaded content"""

    client = _get_service_api_client(context, args)
    result = client.navigate('portal', 'content').GET()
    contents = result.DATA.get('PortalFileInfoList', [])

    if len(contents) == 0:
        print("[WARNING] No content uploaded content found")
    else:
        hierarchy = _generate_hierarchy(contents)
        _show_hierarchy(hierarchy)


def command_show_uploaded(context, args) -> None:
    """
    List all uploaded content in the DLC bucket
    """
    versioned = content_bucket.content_versioning_enabled(context, context.config.default_deployment)
    if versioned:
        raise RuntimeError("[WARNING] Versioning is not supported for this command. Please use list-file-versions and list-bucket-content")
    _service_call_list_content(context, args)


def _service_call_delete_file(context, args, files=None) -> None:
    """
    Delete a set of uploaded files via the ServiceAPI
    """
    if files is None:
        files = []

    client = _get_service_api_client(context, args)

    if args.confirm_deletion:
        for file in files:
            result = client.navigate('portal', 'info', file).DELETE(body=None)
            print(result)
    else:
        print(f'{files} would be deleted if "confirm-deletion" is set')


def command_delete_uploaded(context, args) -> None:
    """
    Deletes a staged manifest or PAK (plus any children)
    """
    if '.json' in args.file_path:
        print(f"Removing {args.file_path}")
    else:
        print(f"Removing {args.file_path} and any children")
    versioned = content_bucket.content_versioning_enabled(context, context.config.default_deployment)

    if versioned:
        raise RuntimeError("[WARNING] Versioning is not supported for this command. Please use clear-dynamic-content")
    else:
        files = [args.file_path]
        children = staging.get_children_by_parent_name_and_version(context, args.file_path, args.version_id)
        for child in children:
            files.append(child.get('FileName', {}).get('S'))
        _service_call_delete_file(context, args, files=files)


def command_show_log_events(context, args) -> None:
    stack_id = __get_service_stack_id(context, args)
    service = ServiceLogs(stack_id=stack_id, context=context)

    # change default look back period if set
    if args.minutes:
        service.period = args.minutes

    resource_region = resource_manager.util.get_region_from_arn(stack_id)
    session = boto3.Session(region_name=resource_region)
    logs = (service.get_logs(session=session))

    if len(logs) == 0:
        print('No logs found')

    for log in logs:
        print(log)

