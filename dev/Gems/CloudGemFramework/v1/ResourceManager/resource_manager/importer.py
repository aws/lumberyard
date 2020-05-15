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
from botocore.exceptions import EndpointConnectionError
from botocore.exceptions import ClientError

from . import util
from .importer_class import DynamoDBImporter
from .importer_class import S3Importer
from .importer_class import LambdaImporter
from .importer_class import SQSImporter
from .importer_class import SNSImporter

from .errors import HandledError


def importer_generator(type, region, context):
    try:
        typeOriginal = type
        type = type.lower()
        if type == 'dynamodb':
            resource_importer = DynamoDBImporter(region, context)
        elif type == 's3':
            resource_importer = S3Importer(region, context)
        elif type == 'lambda':
            resource_importer = LambdaImporter(region, context)
        elif type == 'sqs':
            resource_importer = SQSImporter(region, context)
        elif type == 'sns':
            resource_importer = SNSImporter(region, context)
        else:
            raise HandledError('Type {} is not supported.'.format(typeOriginal))
    except (EndpointConnectionError, ClientError) as e:
        raise HandledError(str(e))

    return resource_importer

def import_resource(context,args):
    if args.type is not None:
        type = args.type
    else:
        try:
            type = args.arn.split(':')[2]
        except IndexError:
            raise HandledError('Invalid ARN {}.'.format(args.arn))

    validationInfo = context.config.validate_resource(type, 'name', args.resource_name)
    if validationInfo['isValid'] is False:
        raise HandledError('Invalid resource name: {}'.format(validationInfo['help']))

    region = util.get_region_from_arn(args.arn)
    resource_importer = importer_generator(type, region, context)
    resource_importer.add_resource(args.resource_name, args.resource_group, args.arn, args.download, context)

def list_aws_resources(context,args):
    if args.region is not None:
        region = args.region
    elif context.config.project_stack_id:
        region = util.get_region_from_arn(context.config.project_stack_id)
    else:
        raise HandledError('Region is required.')
    resource_importer = importer_generator(args.type, region, context)
    resource_importer.list_resources(context)
