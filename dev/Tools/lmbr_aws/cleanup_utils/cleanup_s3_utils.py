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
import botocore

from botocore.exceptions import ClientError

from . import exception_utils
from . import cleanup_utils


def global_resource(cleanup_function):
    def wrapper(_cleaner):
        if _cleaner.delete_global_resources:
            cleanup_function(_cleaner)
        else:
            print("Skipping deletion of global {} (use --delete-global-resources or exclude --region to delete)".
                  format(cleanup_function.__name__[len("_delete_"):]))

    return wrapper


@global_resource
def delete_s3_buckets(cleaner):
    """
    Deletes all buckets from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    # Note: not a paginated API
    # https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/s3.html#paginators
    # list_buckets() does not have a pagination parameter, nor does it specify how many buckets are returned
    # Construct list
    print('\n\nlooking for buckets with names starting with one  {}'.format(cleaner.describe_prefixes()))
    bucket_name_list = []

    try:
        response = cleaner.s3.list_buckets()

        for bucket in response['Buckets']:
            bucket_name = bucket['Name']
            if cleaner.has_prefix(bucket_name):
                print('  found bucket {}'.format(bucket_name))
                bucket_name_list.append(bucket_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting s3 buckets. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the s3 client. {}".format(exception_utils.message(e)))
        return

    # Delete buckets
    for bucket_name in bucket_name_list:
        _clean_s3_bucket(cleaner, bucket_name)
        try:
            print('    deleting bucket'.format(bucket_name))
            cleaner.s3.delete_bucket(Bucket=bucket_name)
        except ClientError as e:
            print('      ERROR: Failed to delete bucket {0} due to {1}'.format(bucket_name, exception_utils.message(e)))
            cleaner.add_to_failed_resources('s3', bucket_name)

    # Wait for buckets to delete
    for bucket_name in bucket_name_list:
        waiter = cleaner.s3.get_waiter('bucket_not_exists')
        try:
            waiter.wait(Bucket=bucket_name,
                        WaiterConfig={'Delay': cleaner.wait_interval, 'MaxAttempts': cleaner.wait_attempts})
        except botocore.exceptions.WaiterError as e:
            if cleanup_utils.WAITER_ERROR_MESSAGE in exception_utils.message(e):
                print("      ERROR: Timed out waiting for bucket {} to delete".format(bucket_name))
            else:
                print("      ERROR: Unexpected error occurred waiting for bucket {0} to delete due to {1}".format(
                    bucket_name, exception_utils.message(e)))
            cleaner.add_to_failed_resources('s3', bucket_name)


def _clean_s3_bucket(cleaner, bucket_name):
    """
    Deletes all object dependencies in a bucket. The delete function is unique in that it deletes objects in
    batches.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param bucket_name: The name of the s3 bucket to clean.
    :return:
    """
    print('    cleaning bucket {}'.format(bucket_name))
    # Get the first batch
    try:
        response = cleaner.s3.list_object_versions(Bucket=bucket_name, MaxKeys=1000)
    except ClientError as e:
        print('      ERROR: Unexpected error while trying to gather s3 object versions. {}'.format(e))
        cleaner.add_to_failed_resources('s3', bucket_name)
        return

    # Deleting objects in batches is capped at 1000 objects, therefore we can't construct the entire list beforehand
    delete_verification_list = []
    while True:
        delete_list = []
        for version in response.get('Versions', []):
            delete_list.append({'Key': version['Key'], 'VersionId': version['VersionId']})
        for marker in response.get('DeleteMarkers', []):
            delete_list.append({'Key': marker['Key'], 'VersionId': marker['VersionId']})
        delete_verification_list.extend(delete_list)
        try:
            cleaner.s3.delete_objects(Bucket=bucket_name, Delete={'Objects': delete_list, 'Quiet': True})
        except ClientError as e:
            print('        ERROR: Failed to delete objects {0} from bucket {1}. {2}'
                  .format(delete_list, bucket_name, exception_utils.message(e)))
            cleaner.add_to_failed_resources('s3', delete_list)
        next_key = response.get('NextKeyMarker', None)
        if next_key:
            response = cleaner.s3.list_object_versions(Bucket=bucket_name, MaxKeys=1000, KeyMarker=next_key)
        else:
            break

    # Wait for all objects to be deleted
    waiter = cleaner.s3.get_waiter('object_not_exists')
    for deleting_object in delete_verification_list:
        try:
            waiter.wait(Bucket=bucket_name, Key=deleting_object['Key'], VersionId=deleting_object['VersionId'],
                        WaiterConfig={'Delay': cleaner.wait_interval, 'MaxAttempts': cleaner.wait_attempts})
            print('    Finished deleting s3 object with key {}'.format(deleting_object['Key']))
        except botocore.exceptions.WaiterError as e:
            if cleanup_utils.WAITER_ERROR_MESSAGE in exception_utils.message(e):
                print("ERROR: Timed out waiting for s3 object with key {} to delete".format(deleting_object['Key']))
            else:
                print("ERROR: Unexpected error occurred waiting for s3 object with key {0} to delete due to {1}"
                      .format(deleting_object['Key'], exception_utils.message(e)))
            cleaner.add_to_failed_resources('s3', deleting_object['Key'])
