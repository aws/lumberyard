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

import importlib
import boto3
import CloudCanvas
import service
import re
import json
from datetime import datetime
from datetime import timedelta

from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import padding
from botocore.signers import CloudFrontSigner
from botocore.client import Config

## CloudFront
CDN_FOLDER = "CDN"
CDN_KEY_PREFIX = "pk-"
CDN_KEY_SUFFIX = ".pem"

cdn_data_bucket = CloudCanvas.get_setting('CDNAccessBucket')
cdn_name = CloudCanvas.get_setting('Distribution')

#duration in seconds
def _cloudfront_url_duration():
    return 100
    
def _get_cdn_data():
    def key_id_from_name(file_name):
        result = re.search(CDN_FOLDER + '/' + CDN_KEY_PREFIX + '(.*)' + CDN_KEY_SUFFIX, file_name)
        return result.group(1)

    if not hasattr(_get_cdn_data,'key_data'):
        bucket = boto3.resource('s3', config=Config(signature_version='s3v4')).Bucket(cdn_data_bucket)
        cdn_prefix = '{}/{}'.format(CDN_FOLDER, CDN_KEY_PREFIX)
        result_list = bucket.objects.filter(Prefix=cdn_prefix)
        found_key = None
        for obj in result_list:
            if obj.key.endswith(CDN_KEY_SUFFIX):
                found_key = obj.key
                break
        if not found_key:
            raise RuntimeError('No key data found at {}:{}'.format(cdn_data_bucket,cdn_prefix))
        else:
            s3_client = boto3.client('s3', config=Config(signature_version='s3v4'))
            print 'Attempting to get {}'.format(found_key)
            cdn_data = s3_client.get_object(Bucket = cdn_data_bucket, Key=found_key)
            _get_cdn_data.key_data = key_id_from_name(found_key), cdn_data['Body'].read()
    return _get_cdn_data.key_data

# Algorithm based on https://boto3.readthedocs.io/en/latest/reference/services/cloudfront.html#generate-a-signed-url-for-amazon-cloudfront
def get_cdn_presigned(file_name):
    print 'Getting presigned url for {} from CDN - {}'.format(file_name, cdn_name)
    cdn_key_name, cdn_key_data = _get_cdn_data()
    print 'Got key name {} data {}'.format(cdn_key_name, cdn_key_data)

    def _rsa_signer(message):
        private_key = serialization.load_pem_private_key(cdn_key_data, password=None, backend=default_backend())
        signer = private_key.signer(padding.PKCS1v15(), hashes.SHA1())
        signer.update(message)
        return signer.finalize()

    if not hasattr(get_cdn_presigned, 'cdn_domain'):
        client = boto3.client('cloudfront')
        response = client.get_distribution(Id=cdn_name)
        print 'CDN {} got distribution info {}'.format(cdn_name, response)

        get_cdn_presigned.cdn_domain = response.get('Distribution',{}).get('DomainName')

    if not get_cdn_presigned.cdn_domain:
        print 'No domain on cdn {} to get signed url from'.format(cdn_name)

    current_time = datetime.utcnow()
    expire_time = current_time + timedelta(seconds=_cloudfront_url_duration())

    cloudfront_signer = CloudFrontSigner(cdn_key_name, _rsa_signer)

    url = 'https://' + get_cdn_presigned.cdn_domain + '/' + file_name
    print 'Retrieving signed url for {}'.format(url)

    signed_url = cloudfront_signer.generate_presigned_url(url, date_less_than=expire_time)

    print 'Got signed url of {}'.format(signed_url)
    return signed_url


