from cgf_utils import custom_resource_response
from glue import Glue
from athena import DEFAULT_EVENTS
import sensitivity
import os
import metric_constant as c
import util
import metric_schema as schema
import athena
from thread_pool import ThreadPool

def handler(event, context):
    print "Start Glue"  
    stack_id = event[c.ENV_STACK_ID]
    resources = util.get_stack_resources(stack_id)  
    request_type = event['RequestType']
    db_name = athena.get_database_name(stack_id, False) 
    glue = Glue()  

    for resource in resources:        
        if resource.logical_id == c.RES_SERVICE_ROLE:
           role_name = resource.physical_id
        if resource.logical_id == c.RES_S3_STORAGE:
           storage_physical_id = resource.physical_id
    
    if role_name is None:
        raise errors.ClientError("The logical resource '{}' was not found.  Is the resource in the cloud formation stack?".format(c.RES_SERVICE_ROLE))   

    if storage_physical_id is None:
        raise errors.ClientError("The logical resource '{}' was not found.  Is the resource in the cloud formation stack?".format(c.RES_S3_STORAGE))           
    crawler_id_1 =  glue.get_crawler_name(stack_id)    
    srcs = [
                {
                    'Path': "{}/{}{}".format(storage_physical_id, "table=", DEFAULT_EVENTS.CLIENTINITCOMPLETE),
                    'Exclusions': []
                },
                {
                    'Path': "{}/{}{}".format(storage_physical_id, "table=", DEFAULT_EVENTS.SESSIONSTART),
                    'Exclusions': []
                }
            ]
      

    print request_type, db_name, crawler_id_1, "role: ", role_name, "s3: ", storage_physical_id
    if request_type.lower() == 'delete':
        if glue.get_crawler(crawler_id_1) is not None:       
            glue.stop_crawler(crawler_id_1) 
            glue.delete_crawler(crawler_id_1)

        if glue.database_exists(db_name):
            glue.delete_database(db_name)
    elif request_type.lower() == 'create':   
        if not glue.database_exists(db_name):
            glue.create_database(db_name)

        if glue.get_crawler(crawler_id_1) is None:
            glue.create_crawler(crawler_id_1, role_name, db_name, athena.get_table_prefix(stack_id), srcs=srcs )

    else:                   
        if glue.get_crawler(crawler_id_1) is None:
            glue.create_crawler(crawler_id_1, role_name, db_name, athena.get_table_prefix(stack_id), srcs=srcs )
        else:
            glue.stop_crawler(crawler_id_1) 
            glue.update_crawler(crawler_id_1, role_name, db_name, athena.get_table_prefix(stack_id) )
        
    return custom_resource_response.success_response({}, "*")

def start_crawler(event, context):
    glue = Glue()  
    crawler_id_1 =  glue.get_crawler_name(event)
    thread_pool = ThreadPool()    
    thread_pool.add(glue.start_crawler, crawler_id_1)
    thread_pool.wait()

def stop_crawler(event, context):
    glue = Glue()  
    crawler_id_1 =  glue.get_crawler_name(event)
    thread_pool = ThreadPool()    
    thread_pool.add(glue.stop_crawler, crawler_id_1)
    thread_pool.wait()

def table_schema():
    schema_fields = []
    for field in schema.REQUIRED_ORDER:
        schema_fields.append(field.id, )

def cli(context, args):
    util.set_logger(args.verbose)

    from resource_manager_common import constant
    credentials = context.aws.load_credentials()

    resources = util.get_resources(context)           
    os.environ[c.ENV_REGION] = context.config.project_region 
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""    
    os.environ[c.IS_LOCALLY_RUN] = "True"
    os.environ["AWS_ACCESS_KEY"] = args.aws_access_key if args.aws_access_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.ACCESS_KEY_OPTION)
    os.environ["AWS_SECRET_KEY"] = args.aws_secret_key if args.aws_secret_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.SECRET_KEY_OPTION)
    eval(args.function)({'RequestType': args.event_type, c.ENV_STACK_ID: resources[c.ENV_STACK_ID]}, type('obj', (object,), {'function_name' : resources[c.RES_LAMBDA_FIFOCONSUMER]}))    
