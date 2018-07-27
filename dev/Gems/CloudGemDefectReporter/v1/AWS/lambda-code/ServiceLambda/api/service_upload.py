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

from __future__ import print_function

import uuid
import os
from itertools import chain
import copy
import boto3
from botocore.client import Config
import CloudCanvas
import service
import errors
import defect_reporter_constants as constants
import defect_reporter_s3 as s3



@service.api
def request_presigned_posts(request, request_content=None):
    '''Process request and request-content to return a response that contains a url and two arrays of fields--one for encrypted posts and one for unencrypted posts.'''

    __validate_request_content(request_content)

    number_of_unencrypted_posts = request_content.get('NumberOfUnencryptedPosts')
    number_of_encrypted_posts = request_content.get('NumberOfEncryptedPosts')

    if number_of_encrypted_posts == 0 and number_of_unencrypted_posts == 0:
        raise errors.ClientError('Invalid number of posts requested. (Must request at least one.)')

    bucket_name = __get_bucket_name()
    s3_client = s3.get_client()

    unencrypted_posts = __generate_unencrypted_presigned_posts(s3_client, bucket_name, number_of_unencrypted_posts)
    encrypted_posts = __generate_encrypted_presigned_posts(s3_client, bucket_name, number_of_encrypted_posts)

    return __craft_response(unencrypted_posts, encrypted_posts)


def __generate_unencrypted_presigned_posts(s3_client, bucket_name, number_of_posts):
    ''' Generates pre-signed posts that are unencrypted. '''

    __validate_number_of_posts(number_of_posts, constants.MAXIMUM_NUMBER_OF_UNENCRYPTED_POSTS)

    return [__generate_presigned_post(s3_client, bucket_name) for _ in range(number_of_posts)]


def __generate_encrypted_presigned_posts(s3_client, bucket_name, number_of_posts):
    ''' Generates pre-signed posts that are server-side encrypted(SSE-S3) by passing in extra parameters fields and conditions. '''

    __validate_number_of_posts(number_of_posts, constants.MAXIMUM_NUMBER_OF_ENCRYPTED_POSTS)

    fields = __get_presigned_post_fields()
    conditions = __get_presigned_post_conditions()

    return [__generate_presigned_post(s3_client, bucket_name, fields, conditions) for _ in range(number_of_posts)]


def __craft_response(unencrypted_posts, encrypted_posts):
    ''' Takes in lists of unencrypted and encrypted posts to craft the swagger specified response. '''

    response = {}

    urls = [post['url'] for post in chain(unencrypted_posts, encrypted_posts)]

    is_url_the_same = all(url == urls[0] for url in urls)

    if not is_url_the_same:
        raise RuntimeError('Not all urls are the same. Cannot craft response if all urls are not the same value. Seen urls: {}'.format(urls))

    response['Url'] = urls[0]
    response['EncryptedPresignedPostFieldsArray'] = [__craft_response_fields(encrypted_post['fields']) for encrypted_post in encrypted_posts]
    response['UnencryptedPresignedPostFieldsArray'] = [__craft_response_fields(unencrypted_post['fields']) for unencrypted_post in unencrypted_posts]

    return response


def __validate_attachment_bucket(bucket_name):
    '''Validate bucket where attachments will be stored.'''

    if not bucket_name:
        raise RuntimeError('No bucket defined in settings.')


def __validate_request_content(request_content):
    '''Validate request_content recieved from the client.'''

    if request_content is None:
        raise errors.ClientError('Invalid request_content (Empty)')

    if not request_content.has_key('NumberOfEncryptedPosts'):
        raise errors.ClientError('Invalid request_content (Does not contain necessary keys. Found these keys: {})'.format(request_content.keys()))

    if not request_content.has_key('NumberOfUnencryptedPosts'):
        raise errors.ClientError('Invalid request_content (Does not contain necessary keys. Found these keys: {})'.format(request_content.keys()))


def __validate_number_of_posts(number_of_posts, maximum_number_of_posts):
    '''Validate the number of posts requested by the client.'''

    if number_of_posts is None:
        raise errors.ClientError('Invalid NumberOfEncryptedPosts (Empty)')

    if number_of_posts < 0:
        raise errors.ClientError('Invalid NumberOfEncryptedPosts (Number of posts requested is out of range.)')

    if number_of_posts > maximum_number_of_posts:
        raise errors.ClientError('Invalid request_content (Number of posts requested exceedes maximum limit of {}.)'.format(maximum_number_of_posts))


def __validate_presigned_post(presigned_post, expected_field_keys):
    '''Validate that presigned post is well formed.'''

    if 'url' not in presigned_post:
        raise RuntimeError('Malformed presigned post. (No url key in presigned_post dictionary)')

    if 'fields' not in presigned_post:
        raise RuntimeError('Malformed presigned post. (No fields key in presigned_post dictionary)')

    fields = presigned_post['fields'].keys()

    for key in expected_field_keys:
        if key not in fields:
            raise RuntimeError('Malformed presigned post. No {} key found in generated presigned post. Saw these keys: {}'.format(key, fields))


def __generate_presigned_post(s3_client, bucket_name, fields=None, conditions=None):
    '''
    Validates and returns dictionary with url and form fields generated by boto3 generate_presigned_url.

    Note that copy.deepcopy is used to make each condition, fields and presigned_post into
    it's own object to prevent issues when generating more than one pre-signed post.
    '''

    unique_id = __generate_uuid()

    presigned_post = s3_client.generate_presigned_post(
        Bucket=bucket_name,
        Key=unique_id,
        Fields=copy.deepcopy(fields),
        Conditions=copy.deepcopy(conditions),
        ExpiresIn=constants.PRESIGNED_POST_LIFETIME
    )

    print('[Post] \n{}\n'.format(presigned_post))

    is_encrypted = fields is not None and conditions is not None

    if is_encrypted:
        __validate_presigned_post(presigned_post, constants.ENCRYPTED_FIELD_KEYS)
    else:
        __validate_presigned_post(presigned_post, constants.UNENCRYPTED_FIELD_KEYS)

    return copy.deepcopy(presigned_post)


def __get_bucket_name():
    ''' Retrieve bucket name from Cloud Canvas settings. '''

    bucket_name = CloudCanvas.get_setting('AttachmentBucket')

    __validate_attachment_bucket(bucket_name)

    return bucket_name


def __craft_response_fields(fields):
    ''' Craft a dictionary with expected response fields using the fields returned by generate_presigned_url. '''

    response_fields = {}

    response_fields['AmzAlgorithm'] = fields['x-amz-algorithm']

    if 'x-amz-server-side-encryption' in fields:
        response_fields['AmzServerSideEncryption'] = fields['x-amz-server-side-encryption']
        
    response_fields['AmzDate'] = fields['x-amz-date']
    response_fields['Key'] = fields['key']
    response_fields['AmzSignature'] = fields['x-amz-signature']
    response_fields['AmzSecurityToken'] = fields['x-amz-security-token']
    response_fields['Policy'] = fields['policy']
    response_fields['AmzCredential'] = fields['x-amz-credential']

    return response_fields


def __get_presigned_post_fields():
    ''' Returns pre-filled form fields for generate_presigned_post to build on. '''

    return {'x-amz-server-side-encryption': 'AES256'}


def __get_presigned_post_conditions():
    ''' Returns list of conditions to include in the policy when generating pre-signed post. '''

    return [{'x-amz-server-side-encryption': 'AES256'}]


def __generate_uuid():
    '''Generates universally unique identifier using uuid4 to guarantee uniqueness without exposing computer network address.'''

    return str(uuid.uuid4())
