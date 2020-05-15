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

from . import cleanup_utils
from . import exception_utils


def __rest_api_exists(cleaner, rest_api_id):
    """
    Verifies if a rest api exists. This is should be replaced once api supports Waiter objects for api deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param rest_api_id: Can be retrieved from the boto3 get_rest_apis() with response['items']['name']
    :return: True if the api exists, False if it doesn't exist or an error occurs.
    """
    try:
        cleaner.apigateway.get_rest_api(restApiId=rest_api_id)
    except cleaner.apigateway.exceptions.NotFoundException:
        return False
    except ClientError as err:
        print("ERROR: Unexpected error occurred when checking if rest api {0} exists due to {1}"
              .format(rest_api_id, exception_utils.message(err)))
        return False
    return True


def delete_api_gateway(cleaner):
    """
    Deletes all api gateways from the client. It will construct a list of resources, delete them, then wait for
    each deletion.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for api gateway resources starting with one of {}'.format(cleaner.describe_prefixes()))
    rest_api_list = []

    # Construct list
    try:
        api_paginator = cleaner.apigateway.get_paginator('get_rest_apis')
        api_page_iterator = api_paginator.paginate()
        
        for page in api_page_iterator:
            for rest_api in page['items']:
                api_name = rest_api['name']
                if cleaner.has_prefix(api_name):
                    print('  found api {}'.format(api_name))
                    rest_api_list.append(rest_api)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting api gateways. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the api gateway client. {}".format(
            exception_utils.message(e)))
        return

    # Delete apis
    for rest_api in rest_api_list:
        print('  deleting rest_api {} id:{}'.format(rest_api['name'], rest_api['id']))
        try:
            cleaner.apigateway.delete_rest_api(restApiId=rest_api['id'])
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print('      ERROR: {}'.format(e))
                cleaner.add_to_failed_resources("apigateway", rest_api['name'])

    # Wait for apis to delete
    for rest_api in rest_api_list:
        try:
            cleanup_utils.wait_for(lambda: not __rest_api_exists(cleaner, rest_api['id']),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. api {0} was not deleted after timeout'.format(rest_api['id']))
            cleaner.add_to_failed_resources("apigateway", rest_api['id'])
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for api {0} to delete due to {1}".format(
                    rest_api['id'], e))
                cleaner.add_to_failed_resources("apigateway", rest_api['id'])
