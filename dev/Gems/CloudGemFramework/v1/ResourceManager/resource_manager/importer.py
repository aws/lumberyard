import json
import util
import botocore.exceptions

from importer_class import ResourceImporter
from importer_class import DynamoDBImporter
from importer_class import S3Importer
from importer_class import LambdaImporter
from importer_class import SQSImporter
from importer_class import SNSImporter

from errors import HandledError
from botocore.exceptions import EndpointConnectionError
from botocore.exceptions import ClientError

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
        raise HandledError(e.message)

    return resource_importer

def import_resource(context,args):
    if args.type != None:
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
    if args.region != None:
        region = args.region
    elif context.config.project_stack_id:
        region = util.get_region_from_arn(context.config.project_stack_id)
    else:
        raise HandledError('Region is required.')
    resource_importer = importer_generator(args.type, region, context)
    resource_importer.list_resources(context)
