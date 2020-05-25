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


def __domain_deprecated(cleaner, swf_domain_name):
    """
    Verifies if a swf domain exists. This is should be replaced once swf supports Waiter objects for domain
    deprecation.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :param swf_domain_name: Can be retrieved from the boto3 list_domains with response['domainInfos']['name']
    :return: True if the domain is deprecated, False if it doesn't exist or an error occurs.
    """
    response = None
    try:
        response = cleaner.swf.describe_domain(name=swf_domain_name)
    except cleaner.swf.exceptions.UnknownResourceFault:
        # The domain doesn't exist and therefore is deprecated
        print('    ERROR: Attempted to check if domain {} is deprecated, but could not find it.'
              .format(swf_domain_name))
        return True
    except ClientError as er:
        print('    ERROR: Unexpected error occurred while trying to describe domain {0}. {1}'.format(
            swf_domain_name, exception_utils.message(er)))
    try:
        if response and response['domainInfo']['status'] == 'DEPRECATED':
            return True
    except KeyError:
        print('    ERROR: Unexpected KeyError occurred while trying to check the status for domain {0}.\nResponse: {1}'
              .format(swf_domain_name, response))
    return False


def delete_swf_resources(cleaner):
    """
    Deprecates all swf resources from the client. It will construct a list of resources, delete them, then wait for
    each deprecation.
    :param cleaner: A Cleaner object from the main cleanup.py script
    :return: None
    """
    print('\n\nlooking for SWF resources with names starting with one of {}'.format(cleaner.describe_prefixes()))
    swf_domain_list = []
    
    # Construct list
    try:
        swf_domain_paginator = cleaner.swf.get_paginator('list_domains')
        swf_domain_page_iterator = swf_domain_paginator.paginate(registrationStatus='REGISTERED')

        for page in swf_domain_page_iterator:
            for domain in page['domainInfos']:
                print('Domain found: {}'.format(domain))
                domain_name = domain['name']
                if cleaner.has_prefix(domain_name):
                    print('  found domain {}'.format(domain_name))
                    swf_domain_list.append(domain_name)
    except KeyError as e:
        print("      ERROR: Unexpected KeyError while deleting swf domains. {}".format(exception_utils.message(e)))
        return
    except ClientError as e:
        print("      ERROR: Unexpected error for paginator for the swf client. {}".format(exception_utils.message(e)))
        return

    # Deprecate domains
    for domain_name in swf_domain_list:
        print('  deprecating SWF domain {}'.format(domain_name))
        try:
            cleaner.swf.deprecate_domain(name=domain_name)
        except cleaner.swf.exceptions.UnknownResourceFault or cleaner.swf.exceptions.DomainDeprecatedFault:
            print('     WARNING: the domain {} does not exist or is already deprecated'.format(domain_name))
        except ClientError as e:
            print('    ERROR: Unexpected error occurred while trying to deprecate domain {0}. {1}'
                  .format(domain_name, exception_utils.message(e)))
            cleaner.add_to_failed_resources("swf", domain_name)

    # Wait for domains to be deprecated
    for domain_name in swf_domain_list:
        try:
            cleanup_utils.wait_for(lambda: not __domain_deprecated(cleaner, domain_name),
                                   attempts=cleaner.wait_attempts,
                                   interval=cleaner.wait_interval,
                                   timeout_exception=cleanup_utils.WaitError)
        except cleanup_utils.WaitError:
            print('      ERROR. domain {0} was not deprecated after timeout'.format(domain_name))
            cleaner.add_to_failed_resources("swf", domain_name)
        except ClientError as e:
            if e.response["Error"]["Code"] == "TooManyRequestsException":
                print('    too many requests, sleeping...')
                time.sleep(cleaner.wait_interval)
            else:
                print("      ERROR: Unexpected error occurred waiting for domain {0} to deprecate due to {1}"
                      .format(domain_name, exception_utils.message(e)))
                cleaner.add_to_failed_resources("swf", domain_name)
