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
# $Revision: #1 $

import content_manifest
import dynamic_content_migrator
import dynamic_content_settings


def before_this_resource_group_updated(hook, deployment_name, **kwargs):
    dynamic_content_migrator.export_current_table_entries(hook.context, deployment_name)


def after_this_resource_group_updated(hook, deployment_name, **kwargs):
    dynamic_content_migrator.import_table_entries_from_backup(hook.context, deployment_name)

    if not content_manifest.validate_writable(hook.context, None):
        # Not raising here - we want to continue and let the user upload after
        return

    # Pass None to upload whatever is set in the startup/bootstrap manifest and immediately stage it as PUBLIC
    staging_args = {}
    staging_args['StagingStatus'] = 'PUBLIC'

    auto_upload_name = None
    resource_group = hook.context.resource_groups.get(dynamic_content_settings.get_default_resource_group())

    if resource_group:
        auto_upload_name = resource_group.get_editor_setting('DynamicContentDefaultManifest')

    content_manifest.upload_manifest_content(hook.context, auto_upload_name, deployment_name, staging_args)


def gather_writable_check_list(hook, check_list, **kwargs):
    this_path = content_manifest.determine_manifest_path(hook.context, None)
    check_list.append(this_path)
