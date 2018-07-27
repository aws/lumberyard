import time
import metric_constant as c
import os
import logging
import sensitivity
import json
import decimal
import datetime
import botocore
from resource_manager_common import stack_info


class DynamoDbDecoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, decimal.Decimal):
            if o % 1 > 0:
                return float(o)
            else:
                return int(o)
        elif isinstance(o, set):
            return list(o)
        elif isinstance(o, datetime.datetime):
            return unicode(o)
        elif isinstance(o, botocore.response.StreamingBody):
            return str(o)
        return super(DynamoDbDecoder, self).default(o)

class DynamoDbEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, float):
            if o % 1 > 0:
                return decimal.Decimal(o)
            else:
                return int(o)
        elif isinstance(o, set):
            return list(o)
        elif isinstance(o, datetime.datetime):
            return unicode(o)
        elif isinstance(o, botocore.response.StreamingBody):
            return str(o)
        return super(DynamoDbEncoder, self).default(o)

def time_remaining(context):
    return round(context[c.KEY_MAX_LAMBDA_TIME]- elapsed(context),1)


def elapsed(context):
    return round(time.time() - context[c.KEY_START_TIME],2)


def elasped_time_in_min(context):
    return (time.time() - context[c.KEY_START_TIME]) / 60


def path_with_filename(partition, date, uuid, sep):    
    return "{}{}{}_{}.parquet".format(partition, sep, date, uuid) 


def now_as_string():
    return datetime.datetime.utcnow().strftime(path_date_format())


def path_date_format():
    return "%Y%m%d%H%M%S"


def partition_date_format():
    return '%Y%m%d%H0000'


def path_without_filename(partition, schema, sep):
    return "{}{}{}".format(partition, sep, schema) 


def path_to_parts(path, sep):
    parts = path.split(sep)
    partition = sep.join(parts[0:len(parts)-1])
    schema_uuid = parts[len(parts)-1]
    schema_uuid = schema_uuid.split(".")[0].split("_")
    schema = schema_uuid[0]
    partial = schema_uuid[1]
       
    return partition, schema, partial


project_name = None
deployment_name = None
resource_group_name = None

def __set_stack_attributes(stack_arn, use_cache=True):
    global project_name, deployment_name, resource_group_name
    if not use_cache or project_name is None or deployment_name is None or resource_group_name is None:
        stack_manager = stack_info.StackInfoManager()
        stack = stack_manager.get_stack_info(stack_arn)
        deployment_name = stack.deployment.deployment_name
        project_name = stack.deployment.parent_stack.project_name
        resource_group_name = c.RES_GEM_NAME
    return project_name, deployment_name, resource_group_name

def get_project_name(stack_arn, use_cache=True):
    proj, deployment, rg_name = __set_stack_attributes(stack_arn, use_cache)
    return proj

def get_deployment_name(stack_arn, use_cache=True):
    proj, deployment, rg_name = __set_stack_attributes(stack_arn, use_cache)
    return deployment

def get_stack_name_from_arn(stack_arn, use_cache=True):
    proj, deployment, rg_name = __set_stack_attributes(stack_arn, use_cache)
    return "{}-{}-{}".format(proj, deployment, rg_name)


def get_resources(context):
    default_deployment = context.config.user_default_deployment
    deployment_arn = context.config.get_deployment_stack_id(default_deployment)
    gem_resource_group = context.stack.describe_resources(deployment_arn, recursive=True)          
    resources = dict({})           
    resources[c.ENV_STACK_ID] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_S3_STORAGE)]['StackId']
    resources[c.RES_DB_TABLE_CONTEXT] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_DB_TABLE_CONTEXT)]['PhysicalResourceId']       
    resources[c.RES_S3_STORAGE] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_S3_STORAGE)]['PhysicalResourceId']   
    resources[c.RES_LAMBDA_FIFOCONSUMER] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_LAMBDA_FIFOCONSUMER)]['PhysicalResourceId']   
    resources[c.RES_LAMBDA_FIFOPRODUCER] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_LAMBDA_FIFOPRODUCER)]['PhysicalResourceId']       
    resources[c.RES_AMOEBA_1] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_AMOEBA_1)]['PhysicalResourceId']       
    resources[c.RES_AMOEBA_2] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_AMOEBA_2)]['PhysicalResourceId']       
    resources[c.RES_AMOEBA_3] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_AMOEBA_3)]['PhysicalResourceId']       
    resources[c.RES_AMOEBA_4] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_AMOEBA_4)]['PhysicalResourceId']       
    resources[c.RES_AMOEBA_5] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_AMOEBA_5)]['PhysicalResourceId']       
    resources[c.RES_SERVICE_ROLE] = gem_resource_group['{}.{}'.format(c.RES_GEM_NAME, c.RES_SERVICE_ROLE)]['PhysicalResourceId']       

    return resources


def get_cloudwatch_namespace(arn):    
    return "{}/{}".format(c.CW_METRIC_NAMESPACE,get_stack_name_from_arn(arn))


logger = logging.getLogger()
logger.setLevel(logging.INFO)


def debug_print(message):        
    if c.ENV_VERBOSE in os.environ and os.environ[c.ENV_VERBOSE] == "True":        
        if c.IS_LOCALLY_RUN in os.environ and os.environ[c.IS_LOCALLY_RUN] == "True":
            print message
        else:
            logger.info(message)


def split(list, size, func=None):
    sets = []
    set = []
    count = 0
    for entry in list:
        count += 1
        if func:
            set.append(func(entry))
        else:
            set.append(entry)
        if count >= size:            
            sets.append(set)
            set = []
    if len(set)>0:
        sets.append(set)
    return sets


def get_stack_prefix(arn):
    return get_stack_name_from_arn(arn).replace("-","_").lower()


def get_stack_resources(arn):    
    stack_manager = stack_info.StackInfoManager()    
    stack = stack_manager.get_stack_info(arn)

    #one common database name so that all deployments show in the same database under different tables
    return stack.resources


def print_dict(dict):
    print json.dumps(dict)


def print_obj(obj):
   for attr in dir(obj):
       if hasattr( obj, attr ):
           print( "obj.%s = %s" % (attr, getattr(obj, attr)))