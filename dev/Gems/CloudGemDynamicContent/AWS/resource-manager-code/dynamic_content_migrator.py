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

import json
import os

import content_bucket
import dynamic_content_settings
import staging
from resource_manager.deployment import delete_tags

BACKUP_FILE_SUFFIX = '_unversioned_data.backup'
UNVERSIONED_TABLE = 'StagingSettingsTable'
VERSIONED_TABLE = 'VersionedStagingSettingsTable'


def command_migrate_table_entries(context: object, args: dict):
    """
        Migrate existing staging settings information when content versioning is enabled or suspended

        Arguments
        context -- context to use
        args -- arguments from the CLI command

    """
    deployment_name = args.deployment_name if args.deployment_name else context.config.default_deployment
    export_current_table_entries(context, deployment_name)
    import_table_entries_from_backup(context, deployment_name)


def command_suspend_versioning(context: object, args: dict):
    """
        Suspend dynamic content versioning

        Arguments
        context -- context to use
        args -- arguments from the CLI command

    """
    deployment_name = args.deployment_name if args.deployment_name else context.config.default_deployment
    if not content_bucket.content_versioning_enabled(context, deployment_name):
        print('Content versioning is not enabled')
        return

    message = 'Suspend versioning to stop accruing new versions of the same object in the content bucket. ' \
              'Existing objects in the content bucket do not change but the staging settings for any noncurrent version will be lost. ' \
              'Update your deployment after running this command for the suspension to take effect.'
    if not args.confirm_versioning_suspension:
        context.view.confirm_message(message)

    delete_tags(context, deployment_name, ['content-versioning'])


def export_current_table_entries(context: object, deployment_name: str):
    """
        Export existing table entries to a local backup file

        Arguments
        context -- context to use
        deployment_name -- name of the deployment

    """
    try:
        versioned = content_bucket.content_versioning_enabled(context, deployment_name)
    except Exception as e:
        if str(e) == 'No stack_id provided.':
            return
        raise e

    current_table = staging.get_staging_table(context, deployment_name, versioned)
    if not current_table:
        return

    client = context.aws.client('dynamodb')
    try:
        # Scan all the data in the current table
        response = client.scan(
            TableName=current_table)
        entries = response.get('Items', [])

        while 'LastEvaluatedKey' in response:
            response = client.scan(
                TableName=current_table,
                ExclusiveStartKey=response['LastEvaluatedKey'])
            entries += response.get('Items', [])
    except Exception as e:
        print(f'Failed to scan current table entries. Error: {e}')
        return

    num_entries = len(entries)
    if not num_entries:
        print('No table entry was found.')
        return
    print(f'Found {num_entries} table entries.')

    # Save the backup data to local disk
    backup_folder = dynamic_content_settings.get_table_backup_folder(context.config.game_directory_path)
    if not os.path.exists(backup_folder):
        os.makedirs(backup_folder)
    backup_path = os.path.join(backup_folder, deployment_name + BACKUP_FILE_SUFFIX)
    with open(backup_path, 'w+') as backup:
        backup_content = {'versioned': versioned, 'entries': entries}
        json.dump(backup_content, backup)


def import_table_entries_from_backup(context: object, deployment_name: str):
    """
        Load table entries from a local backup file and import them to the current table

        Arguments
        context -- context to use
        deployment_name -- name of the deployment

    """
    backup_folder = dynamic_content_settings.get_table_backup_folder(context.config.game_directory_path)
    backup_path = os.path.join(backup_folder, deployment_name + BACKUP_FILE_SUFFIX)
    if not os.path.exists(backup_path):
        print(f'Backup file {backup_path} does not exist. No data to migrate')
        return
    with open(backup_path, 'r') as backup:
        # Load backup data from local disk
        backup_content = json.load(backup)

    versioned = content_bucket.content_versioning_enabled(context, deployment_name)
    if backup_content['versioned'] == versioned:
        # Version status doesn't change before and after the deployment update
        # No need to migrate any data in this case
        __remove_backup_file(backup_path)
        return

    entries = backup_content['entries']

    current_table = staging.get_staging_table(context, deployment_name, versioned)
    client = context.aws.client('dynamodb')
    entries_with_failure = []
    for entry in entries:
        if versioned:
            entry['VersionId'] = {'S': content_bucket.get_latest_version_id(context, entry['FileName']['S'], deployment_name)}
            if entry.get('Parent', {}).get('S'):
                entry['ParentVersionIds'] = {'SS': [content_bucket.get_latest_version_id(context, entry['Parent']['S'], deployment_name)]}
        else:
            if not content_bucket.is_latest_version(context, entry['FileName']['S'], entry['VersionId']['S'], deployment_name):
                # After content versioning is suspended, we should only migrate the staging status
                # for the latest versions of files
                continue
            entry.pop('VersionId', None)
            entry.pop('ParentVersionIds', None)

        try:
            # Check whether the key already exists in the current table
            key = {'FileName': entry['FileName'], 'VersionId': entry['VersionId']} if versioned else {'FileName': entry['FileName']}
            existing_item = client.get_item(
                TableName=current_table,
                Key=key).get('Item', {})
        except Exception as e:
            print(f'Failed to get table entry with key {key}. Error: {e}')
            entries_with_failure.append(entry)
            continue
        if existing_item:
            print(f'Table entry with key {key} already exists.')
            entries_with_failure.append(entry)
            continue

        try:
            # Migrate the entry to the current table
            client.put_item(
                TableName=current_table,
                Item=entry)
            print(f'Migrated table entry with key {key}.')
        except Exception as e:
            print(f'Failed to migrate table entry with key {key}. Error: {e}')
            entries_with_failure.append(entry)

    if len(entries_with_failure):
        with open(backup_path, 'w+') as backup:
            json.dump(entries_with_failure, backup)
        print('No all entries were migrated successfully. Please retry the migration with the migrate-unversioned-data command.')
    else:
        __remove_backup_file(backup_path)


def __remove_backup_file(backup_path):
    """
        Delete the local backup file and the backup folder if empty

        Arguments
        backup_path -- path to the backup file

    """
    if os.path.isfile(backup_path):
        os.remove(backup_path)

    backup_folder = os.path.dirname(backup_path)
    if os.path.isdir(backup_folder) and not os.listdir(backup_folder):
        os.rmdir(backup_folder)
