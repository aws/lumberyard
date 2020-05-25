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


def __sqs_queue_exists(cleaner, sqs_queue_url):
    """
    Verifies if a queue exists. This is should be replaced once glue supports Waiter objects for queues.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param sqs_queue_url: Can be retrieved from the boto3 list_queues() with response['QueueUrls']
    :return: True if the database exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.sqs.get_queue_attributes(QueueUrl=sqs_queue_url)
    except cleaner.sqs.exceptions.ClientError as err:
        # Look for a specific ClientError message to validate that the address is invalid
        if 'InvalidAddress' in err.message:
            return False
        # ClientError's are broad, so we want to re-raise if it's not the correct error we're looking for
        else:
            raise err
    return True


def delete_sqs_queues(cleaner):
    """
    Deletes all sqs queues from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    # Note: Not a pageable API
    # https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/sqs.html#paginators
    # list_queues() does not have any paging parameters and simply returns the first 1000 items it finds
    # Construct list
    
    print('\n\nlooking for SQS queues with names starting with one of {}'.format(cleaner.describe_prefixes()))
    queue_url_list = []
    try:
        response = cleaner.sqs.list_queues()
        if 'QueueUrls' not in response:
            print('No queues available, skipping sqs queues')
            return
        for queue_url in response['QueueUrls']:
            queue_parts = queue_url.split("/")
            queue = queue_parts[4]
            if cleaner.has_prefix(queue):
                print('  found queue'.format(queue))
                queue_url_list.append(queue_url)
    except IndexError as e:
        print('      ERROR: Unexpected IndexError from queue url {0}. {1}'.format(queue_url,
                                                                                  exception_utils.message(e)))
        return
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting sqs queues. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for list_queues() for the sqs client. {}".format(
            exception_utils.message(e)))
        return

    # Delete queues
    for queue_url in queue_url_list:
        try:
            print('    deleting queue {}'.format(queue_url))
            cleaner.sqs.delete_queue(QueueUrl=queue_url)
        except ClientError as e:
            if e.response["Error"]["Code"] == "LimitExceededException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: Failed to delete {0}. {1}'.format(queue_url, e))
                cleaner.add_to_failed_resources('sqs', queue_url)

    # Wait for deletions
    for queue_url in queue_url_list:
        try:
            cleanup_utils.wait_for(lambda: not __sqs_queue_exists(cleaner, queue_url),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. sqs queue {0} was not deleted after timeout'.format(queue_url))
            cleaner.add_to_failed_resources("sqs", queue_url)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for sqs queue {0} to delete due to {1}"
                      .format(queue_url, exception_utils.message(e)))
                cleaner.add_to_failed_resources("sqs", queue_url)
