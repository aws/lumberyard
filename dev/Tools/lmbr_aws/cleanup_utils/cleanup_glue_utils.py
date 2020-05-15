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
#
import time

from botocore.exceptions import ClientError

from . import exception_utils
from . import cleanup_utils


def __glue_crawler_exists(cleaner, glue_crawler_name):
    """
    Verifies if a crawler exists. This is should be replaced once glue supports Waiter objects for crawlers.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param glue_crawler_name: Can be retrieved from the boto3 get_crawlers() with response['Crawlers']['Name']
    :return: True if the crawler exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.glue.get_crawler(Name=glue_crawler_name)
    except cleaner.glue.exceptions.EntityNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if crawler {0} exists due to {1}"
              .format(glue_crawler_name, exception_utils.message(err)))
        return False
    return True


def delete_glue_crawlers(cleaner):
    """
    Deletes all glue crawlers from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    if not cleaner.glue:
        print('No AWS Glue client available, skipping glue crawlers')
        return

    print('\n\nlooking for Glue crawlers with names starting with one of {0}'.format(cleaner.describe_prefixes()))
    params = {}
    glue_crawler_name_list = []
    
    # Construct list
    try:
        for result in cleanup_utils.paginate(cleaner.glue.get_crawlers, params, 'NextToken', 'NextToken'):
            for crawler_info in result['Crawlers']:
                crawler_name = crawler_info['Name']
                if cleaner.has_prefix(crawler_name):
                    print('  found glue crawler {}'.format(crawler_name))
                    glue_crawler_name_list.append(crawler_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting glue crawlers. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the glue client. {}".format(exception_utils.message(e)))
        return

    # Delete glue crawlers
    for glue_crawler_name in glue_crawler_name_list:
        print('  deleting crawler {}'.format(glue_crawler_name))
        try:
            cleaner.glue.delete_crawler(Name=glue_crawler_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete crawler {0} due to {1}'.format(glue_crawler_name,
                                                                                    exception_utils.message(e)))
                cleaner.add_to_failed_resources("glue", glue_crawler_name)

    # Wait for deletion
    for glue_crawler_name in glue_crawler_name_list:
        try:
            cleanup_utils.wait_for(lambda: not __glue_crawler_exists(cleaner, glue_crawler_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. Glue crawler {0} was not deleted after timeout'.format(glue_crawler_name))
            cleaner.add_to_failed_resources("glue", glue_crawler_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for glue crawler {0} to delete due to {1}"
                      .format(glue_crawler_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("glue", glue_crawler_name)


def __glue_table_exists(cleaner, glue_database_name, glue_table_name):
    """
    Verifies if a table exists. This is should be replaced once glue supports Waiter objects for tables.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param glue_table_name: Can be retrieved from the boto3 get_tables() with response['TableList']['Name']
    :return: True if the table exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.glue.get_table(Name=glue_table_name, DatabaseName=glue_database_name)
    except cleaner.glue.exceptions.EntityNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if table {0} exists due to {1}"
              .format(glue_table_name, exception_utils.message(err)))
        return False
    return True


def _delete_glue_tables(cleaner, database_name):
    """
    Deletes all glue tables from the client before the database is deleted. It will construct a list of resources,
    delete them, then wait for each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param database_name: The name of the glue database that the tables live under
    :return: None
    """
    print('\n\nlooking for Glue tables with names starting with one of {}'.format(cleaner.describe_prefixes()))
    params = {'DatabaseName': database_name}
    glue_table_name_list = []
    # Construct list
    try:
        for result in cleanup_utils.paginate(cleaner.glue.get_tables, params, 'NextToken', 'NextToken'):
            for table_info in result['TableList']:
                table_name = table_info['Name']
                if cleaner.has_prefix(table_name):
                    print('  found glue table {}'.format(table_name))
                    glue_table_name_list.append(table_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting glue tables. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the glue client. {}".format(exception_utils.message(e)))
        return

    # Delete glue crawlers
    for table_name in glue_table_name_list:
        print('  deleting table {}'.format(table_name))
        try:
            cleaner.glue.delete_table(Name=table_name, DatabaseName=database_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete table {0} due to {1}'.format(table_name,
                                                                                  exception_utils.message(e)))
                cleaner.add_to_failed_resources("glue", table_name)

    # Wait for deletion
    for table_name in glue_table_name_list:
        try:
            cleanup_utils.wait_for(lambda: not __glue_table_exists(cleaner, database_name, table_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. Glue crawler {0} was not deleted after timeout'.format(table_name))
            cleaner.add_to_failed_resources("glue", table_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for glue table {0} to delete due to {1}"
                      .format(table_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("glue", table_name)


def __glue_database_exists(cleaner, glue_database_name):
    """
    Verifies if a database exists. This is should be replaced once glue supports Waiter objects for databases.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param glue_database_name: Can be retrieved from the boto3 get_databases() with
        response['DatabaseList']['Name']
    :return: True if the database exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.glue.get_database(Name=glue_database_name)
    except cleaner.glue.exceptions.EntityNotFoundException:
        return False
    except ClientError as err:
        print("      ERROR: Unexpected error occurred when checking if database {0} exists due to {1}"
              .format(glue_database_name, exception_utils.message(err)))
        return False
    return True


def delete_glue_databases(cleaner):
    """
    Deletes all glue databases from the client. Glue tables need to be deleted first before deleting the database.
    It will construct a list of resources, delete them, then wait for each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    if not cleaner.glue:
        print('No glue client available, skipping glue databases')
        return
    print('\n\nlooking for Glue databases with names starting with one of {}'.format(cleaner.describe_prefixes()))
    params = {}
    glue_database_list = []

    # Construct list
    try:
        for result in cleanup_utils.paginate(cleaner.glue.get_databases, params, 'NextToken', 'NextToken'):
            for database_info in result['DatabaseList']:
                database_name = database_info['Name']
                if cleaner.has_prefix(database_name):
                    print('  found glue database {}'.format(database_name))
                    glue_database_list.append(database_name)

    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting glue databases. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the glue client. {}".format(exception_utils.message(e)))
        return

    # Delete glue crawlers
    for database_name in glue_database_list:
        print('  deleting database {}'.format(database_name))
        try:
            _delete_glue_tables(cleaner, database_name)
            cleaner.glue.delete_database(Name=database_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete database {0} due to {1}'.format(database_name,
                                                                                     exception_utils.message(e)))
                cleaner.add_to_failed_resources("glue", database_name)

    # Wait for deletion
    for database_name in glue_database_list:
        try:
            cleanup_utils.wait_for(lambda: not __glue_database_exists(cleaner, database_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. Glue database {0} was not deleted after timeout'.format(database_name))
            cleaner.add_to_failed_resources("glue", database_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for glue database {0} to delete due to {1}"
                      .format(database_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("glue", database_name)
