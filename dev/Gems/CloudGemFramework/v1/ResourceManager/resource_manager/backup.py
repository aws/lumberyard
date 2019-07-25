import boto3
import botocore

from cgf_utils import custom_resource_utils
from botocore.client import Config
from errors import HandledError
from multiprocessing.dummy import Pool
import transfer_jobs
import time

THREAD_COUNT = 24
MAX_KEYS_PER_READ = 75

DDB_TYPES = ['AWS::DynamoDB::Table', "Custom::DynamoDBTable"]
S3_TYPES = ['AWS::S3::Bucket']


def restore_bucket(context, args):
    if args.deployment is None:
        if context.config.default_deployment is None:
            raise HandledError(
                'No deployment was specified and there is no default deployment configured.')
        args.deployment = context.config.default_deployment

    __validate_resource_arg(args.resource)
    resource_group_name = args.resource.split('.')[0]
    logical_resource_name = args.resource.split('.')[1]
    resource = __get_resource(
        context, args.deployment, resource_group_name, logical_resource_name)
    if not resource['ResourceType'] in S3_TYPES:
        raise HandledError("{} is not an s3 bucket".format(logical_resource_name))
    
    resource_name = __get_resource_name(context,
                                        args.deployment, resource_group_name, logical_resource_name)
    client = context.aws.session.client(
        's3', region_name=context.config.project_region, config=Config(signature_version='s3v4'))
    __restore_s3(client, args.backup_name, resource_name, context.config.project_region)



def backup_resource(context, args):
    if args.deployment is None:
        if context.config.default_deployment is None:
            raise HandledError(
                'No deployment was specified and there is no default deployment configured.')
        args.deployment = context.config.default_deployment
    if args.resource is None:
        __backup_deployment(context, args)
        return

    __validate_resource_arg(args.resource)
    resource_group_name = args.resource.split('.')[0]
    logical_resource_name = args.resource.split('.')[1]
    if not args.backup_name:
        args.backup_name = "{}.{}.{}".format(
            context.config.get_project_stack_name(), args.deployment, args.resource)

    if args.type == None:
        resource = __get_resource(context, args.deployment, resource_group_name, logical_resource_name)
        if resource['ResourceType'] in DDB_TYPES:
            args.type='ddb'
        elif resource['ResourceType'] in S3_TYPES:
            args.type = 's3'
        else:
            raise HandledError("Resource {} is not a supported type".format(logical_resource_name))

    if args.type == "ddb":
        client = context.aws.session.client(
            'dynamodb', region_name=context.config.project_region)
        resource_name = __get_resource_name(context,
            args.deployment, resource_group_name, logical_resource_name)
        __backup_ddb(client, resource_name, args.backup_name)
    elif args.type == "s3":
        resource_name = __get_resource_name(context,
                                            args.deployment, resource_group_name, logical_resource_name)
        client = context.aws.session.client(
            's3', region_name=context.config.project_region, config=Config(signature_version='s3v4'))
        __backup_s3(client, resource_name, args.backup_name, context.config.project_region)
    else:
        raise HandledError(
            "Cannot backup resource of type {}".format(args.type))


def __backup_deployment(context, args):
    if not context.config.deployment_stack_exists(args.deployment):
        raise HandledError("Deployment {} does not exist".format(args.deployment))
    if not args.backup_name:
        raise HandledError(
            'For full deployment backup you must specify a backup name')
    print "Backing up whole deployment {}".format(args.deployment)

    resources = __gather_resource_descriptions(context, args.deployment)

    for resource in resources:
        resource_group_name = resource["ResourceGroup"]
        logical_resource_name = resource["LogicalName"]
        backup_name = "{}{}{}".format(args.backup_name, resource_group_name, logical_resource_name)
        if resource['ResourceType'] in DDB_TYPES:
            client = context.aws.session.client(
                'dynamodb', region_name=context.config.project_region)
            resource_name = __get_resource_name(context,
                                                args.deployment, resource_group_name, logical_resource_name)
            __backup_ddb(client, resource_name, backup_name)
        elif resource['ResourceType'] in S3_TYPES:
            client = context.aws.session.client(
                's3', region_name=context.config.project_region, config=Config(signature_version='s3v4'))
            resource_name = __get_resource_name(context,
                                                args.deployment, resource_group_name, logical_resource_name)
            __backup_s3(client, resource_name, backup_name.lower(), context.config.project_region)


def __gather_resource_descriptions(context, deployment):
    descriptions = []
    if not context.config.deployment_stack_exists(deployment):
        raise HandledError("Deployment {} does not exist".format(deployment))

    for resource_group in context.resource_groups.values():
        if not resource_group.is_enabled:
            continue
        resource_group_stack_id = context.config.get_resource_group_stack_id(deployment, resource_group.name)
        resources = context.stack.describe_resources(resource_group_stack_id)
        for resource_name, resource_description in resources.iteritems():
            if resource_description['ResourceType'] in DDB_TYPES + S3_TYPES:
                resource_description['LogicalName'] = resource_name
                resource_description['ResourceGroup'] = resource_group.name
                descriptions.append(resource_description)
    return descriptions


def __backup_ddb(client, table_name, backup_name):
    print "Backing up {} to {}".format(table_name, backup_name)
    response = client.create_backup(
        TableName=table_name, BackupName=backup_name)
    if 'BackupDetails' in response:
        print "Backup {} was created successfully ({})".format(response['BackupDetails']['BackupName'], response['BackupDetails']['BackupArn'])


def __backup_s3(client, bucket_name, backup_bucket, region_name):
    print "Backing up {} to {}".format(bucket_name, backup_bucket.lower())
    __transfer_s3(client, bucket_name, backup_bucket.lower(), region_name)


def __restore_s3(client, source_bucket, target_bucket, region_name):
        print "Restoring {} from {}".format(target_bucket, source_bucket)
        __transfer_s3(client, source_bucket, target_bucket, region_name)


def __transfer_s3(client, source_bucket, target_bucket, region_name):
    if not __bucket_exists(client, target_bucket):
        __create_bucket(client, target_bucket, region_name)
    t_mgr = transfer_jobs.TransferJobManager(Pool(THREAD_COUNT))
    start = time.time()
    key_read_response = client.list_objects_v2(
        Bucket=source_bucket, MaxKeys=MAX_KEYS_PER_READ)
    jobs = [transfer_jobs.CreateTransferJob(obj, source_bucket, target_bucket)
            for obj in key_read_response.get("Contents", [])]
    t_mgr.add_jobs(jobs)
    while key_read_response["IsTruncated"]:
        key_read_response = client.list_objects_v2(
            Bucket=source_bucket, ContinuationToken=key_read_response[
                "NextContinuationToken"],
            MaxKeys=MAX_KEYS_PER_READ)
        jobs = [transfer_jobs.CreateTransferJob(
            obj, source_bucket, target_bucket) for obj in key_read_response["Contents"]]
        t_mgr.add_jobs(jobs)

    t_mgr.close_pool()
    print "all s3 transfer jobs fired off. waiting for returns"
    t_mgr.wait_for_finish()
    finish = time.time() - start
    print "results:"
    error_count = 0
    for result in t_mgr.concatenate_results():
        if "TrasnferException" in result:
            print "Transfer Error in {}: {}".format(result["Key"], result["TrasnferException"])
            error_count = error_count + 1
    print "transfer completed with {} errors".format(error_count)
    print "finished in {} seconds".format(finish)


def __get_resource(context, deployment, resource_group, resource_name):
    if not context.config.deployment_stack_exists(deployment):
        raise HandledError("Deployment {} does not exist".format(deployment))
    resource_group_stack_id = context.config.get_resource_group_stack_id(
        deployment, resource_group)
    resources = context.stack.describe_resources(resource_group_stack_id)
    description = resources.get(resource_name)
    if description is None:
        raise HandledError("Could not find resource {} in resource group {} in deployment{}".format(
            resource_name, resource_group, deployment))
    return description


def __get_resource_name(context, deployment, resource_group, resource_name):
    description = __get_resource(context, deployment, resource_group, resource_name)
    phys_id = description.get('PhysicalResourceId', None)
    if (description['ResourceType'].startswith("Custom::")):
        return custom_resource_utils.get_embedded_physical_id(phys_id)
    return phys_id


def __validate_resource_arg(resource):
    if not resource.count('.') == 1:
        raise HandledError(
            "Resource arg is not in the <resource_group>.<resource> format")
    split_arg = resource.split('.')
    if len(split_arg) != 2:
        raise HandledError(
            "Resource arg is not in the <resource_group>.<resource> format")
    for item in split_arg:
        if len(item) == 0:
            raise HandledError(
                "Resource arg is not in the <resource_group>.<resource> format")


def __bucket_exists(client, bucket_name):
    try:
        client.head_bucket(
            Bucket=bucket_name
        )
    except botocore.exceptions.ClientError as e:
        return False
    return True


def __create_bucket(client, bucket_name, region_name):
    client.create_bucket(
        Bucket=bucket_name,
        CreateBucketConfiguration={
            'LocationConstraint': region_name
        }
    )
