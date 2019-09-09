import os
import boto3
from botocore.client import Config

def get_s3_client():
    if not hasattr(get_s3_client, 'client'):
        current_region = os.environ.get('AWS_REGION')
        if current_region is None:
            raise RuntimeError('AWS region is empty')

        configuration = Config(signature_version="s3v4", s3={'addressing_style': 'path'})
        get_s3_client.client = boto3.client('s3', region_name=current_region, config=configuration)

    return get_s3_client.client




