from cgf_utils import custom_resource_response
from athena import Query, Athena
from glue import Glue
from dynamodb import DynamoDb
from thread_pool import ThreadPool
from recovery import CreateDDL, CreateTable, DropTable, RepairTable, Cleanup, ValidateTableData
from s3crawler import Crawler
from datetime import date, timedelta
from glue import Glue
import recovery
import metric_constant as c
import os
import util  
import athena
import time
import datetime
import metric_schema

"""
Main entry point for the scheduled glue crawler

Lambdas triggered by growth > threshold use the process function as the entry point
"""
def main(event, request):
    context = dict({})
    context[c.KEY_LAMBDA_FUNCTION] = request.function_name if hasattr(request, 'function_name') else None
    context[c.KEY_REQUEST_ID] = request.aws_request_id if hasattr(request, 'aws_request_id') else None    
    stackid = os.environ[c.ENV_DEPLOYMENT_STACK_ARN]
    
    context[c.KEY_DB] = DynamoDb(context)
    context[c.KEY_ATHENA_QUERY] = Query(stackid)    
    context[c.KEY_GLUE_CRAWLER] = Glue()       
    thread_pool = ThreadPool(size=3)
    crawler_name = context[c.KEY_GLUE_CRAWLER].get_crawler_name(stackid)    
    crawler = Crawler(context, os.environ[c.ENV_S3_STORAGE]) 
    glue = Glue()
    events = glue.get_events()
    
    start = datetime.datetime.utcnow() - datetime.timedelta(hours=2)
    now = datetime.datetime.utcnow()    
    
    found = False
    for type in events:        
        dt = start
        while dt <= now:                                                          
            prefix = metric_schema.s3_key_format().format(context[c.KEY_SEPERATOR_PARTITION], dt.year, dt.month, dt.day, dt.hour, type, dt.strftime(util.partition_date_format()))    
            found = crawler.exists(prefix) 
            if found: 
                print "FOUND new events=>", prefix
                break                
            dt += timedelta(hours=1)
        if found: 
            break
    
    if found:         
        thread_pool.add(crawl, context, crawler_name, context[c.KEY_ATHENA_QUERY].execute_with_format)        
        thread_pool.wait()             
    
    return custom_resource_response.success_response({}, "*")

def crawl(context, crawler_name, query_parser):    
    result = context[c.KEY_GLUE_CRAWLER].get_crawler(crawler_name)
    state = result['Crawler']['State']
    if state != "READY":
        print "Crawler '{}' is currently '{}'.  Quitting lambda.".format(crawler_name, state)
        return

    print "Starting crawler", crawler_name    
    context[c.KEY_GLUE_CRAWLER].start_crawler(crawler_name, sync=False)   

"""
The GLUE crawler does not update partitions
"""
def repair_partitions(context, crawler_name, query_parser):
    print "Updating indexes for '{}'".format(crawler_name)    
    #run the index repair on external tables if it is not currently running.
    context[recovery.STATE_CONTEXT_QUERY] = query_parser     
    query_id = recovery.db_key(crawler_name, "msck_query_id")         
    if query_id in context:        
        id = context[query_id]                            
        query = context[c.KEY_ATHENA_QUERY].client.get_query_execution(id)            
        if query['Status']['State'] == "SUCCEEDED":
            del context[query_id]                    
    repair = RepairTable(context)       
    repair.execute(query_id)        

"""
Primitive state machine.  Uses Dynamodb to maintain the lambda state
"""
def recover(context, crawler_name, query_parser):    
    db_client = context[c.KEY_DB]    
    state_context =__create_state_context(context, crawler_name, query_parser)

    recovery_states = [CreateDDL(state_context), 
                        DropTable(state_context), 
                        CreateTable(state_context), 
                        RepairTable(state_context), 
                        Cleanup(state_context)]
    
    idx = int(context[crawler_name]) if crawler_name in context else 0            
    result = None
    #iterate the states    
    while idx < len(recovery_states):                
        query_id = recovery.db_query_id(crawler_name, idx, "query_id")   
        state = recovery_states[idx]
        print state, 'is EXECUTING' 
        result = state.execute(query_id)
        if result != None:
            idx+=1
            db_client.set(crawler_name, idx)
            db_client.delete_item(query_id)
        time.sleep(10)            
    db_client.delete_item(crawler_name)
    return

"""
The partition repair process sometimes generates odd tables.   This process will remove those extra tables.
"""
def cleanup_crawler_broken_partition_tables(query):
    token = None
    except_list = []
    
    tables = query.execute("SHOW TABLES")
    print tables
    for table in tables:            
        name = table[0]   
        if name not in except_list and query.prefix in name:
            query.execute("DROP TABLE {0}.{1}".format(query.database_name, name),  sync=False)                

def __is_valid_table(context, crawler_name, query_parser):
    if __is_recovery_in_progress(context, crawler_name):
        return True
    
    result = query_parser("SHOW CREATE TABLE {0}.{1}")  
    return len(result) > 0        

def __create_state_context(context, crawler_name, query_parser):
    state_context = {}
    state_context[c.KEY_DB] = context[c.KEY_DB]
    state_context[c.KEY_ATHENA_QUERY] = context[c.KEY_ATHENA_QUERY]
    state_context[c.KEY_GLUE_CRAWLER] = context[c.KEY_GLUE_CRAWLER]
    state_context[recovery.STATE_CONTEXT_CRAWLER_ID] = crawler_name
    state_context[recovery.STATE_CONTEXT_QUERY] = query_parser       
    
    for key in context:
        print "key..", key   
        if crawler_name in key:    
            state_context[ddl_key] = context[ddl_key]
    
    return state_context

def __is_recovery_in_progress(context, crawler_name):
    return recovery.db_key(crawler_name, recovery.STATE_CONTEXT_DDL) in context

def __is_table_data_not_valid(context, crawler_name, query_parser):        
    query_id = recovery.db_key(crawler_name, "count_uid")    
    state_context = __create_state_context(context, crawler_name, query_parser)         
    validate = ValidateTableData(state_context)    
    result = None
    while True:                        
        print validate, 'is EXECUTING' 
        result = validate.execute(query_id)
        if result != None:                        
            context[c.KEY_DB].delete_item(query_id)
            break
        time.sleep(10)            

    if result is None:
        print "Validation query failed.   Validating table exists."          
        if not __is_valid_table(context, crawler_name, query_parser):
            #No table found, which is a valid case.
            print "Table not found"
            return False
        else:
            print "Corrupt table found"
            return True
   
    return False

#ALTER TABLE <table_name> ADD    
#PARTITION (idx_source='cloudgemsamples', idx_bldid='1.0.2') LOCATION '<s3-bucket>/Insensitive/cloudgemmetric/1.0.2/'
#PARTITION (idx_source='cloudgemsamples', idx_bldid='1.0.5432') LOCATION '<s3-bucket>/Insensitive/cloudgemmetric/1.0.5432/'
#PARTITION (idx_source='cloudgemsamples', idx_bldid='2.0.1') LOCATION '<s3-bucket>/Insensitive/cloudgemmetric/2.0.1/'
#MSCK REPAIR TABLE <db_name>.<table_name>;
def __update_partitions(paths):
    alter = StringIO()
    alter.write("ALTER TABLE {0}.{1} ADD ")
    for path in paths:
        if path.sensitivity_level == sensitivity.SENSITIVITY_TYPE.NONE and path.buildid == '1.0.2' and (path.platform == 'Android' or path.platform == 'OSX'):
            alter.write(" PARTITION (idx_source='{1}', idx_bldid='{2}', idx_year='{3}', idx_month='{4}', idx_day='{5}', idx_hour='{6}', idx_platform='{7}', idx_event='{8}') LOCATION 's3://<bucket>/{0}/{1}/{2}/{3}/{4}/{5}/{6}/{7}/{8}/'".format(
                path.sensitivity_level,                
                path.source,
                path.buildid, 
                path.year, 
                path.month, 
                path.day, 
                path.hour, 
                path.platform,
                path.event              
            ))
        
    query = Query(type('obj', (object,), {c.ENV_STACK_ID: os.environ[c.ENV_DEPLOYMENT_STACK_ARN]}))
    query.execute(alter.getvalue())

def __update_partitions2(paths):
    to_delete =  []    
    db_name = ""
    table_name = "" 
    for path in paths:                        
        to_delete.append(path.source)
        to_delete.append(path.buildid)
        to_delete.append(str(path.year))
        to_delete.append(str(path.month))
        to_delete.append(str(path.day))
        to_delete.append(str(path.hour))
        to_delete.append(path.platform)
        to_delete.append(path.event)
        to_delete.append(path.schema)               
        break         
        
    glue = Glue()
    print glue.delete_partitions(db_name, table_name, to_delete)

def __get_partitions():
    to_delete =  []    
    db_name = ""
    table_name = "" 
   
    glue = Glue()    
    token = None
    partitions_to_delete = []
    while True:
        response = glue.get_partitions(db_name, table_name, token)        
        token = response["NextToken"] if "NextToken" in response else None
        partitions = response["Partitions"]
        idx = 0
        for partition in partitions:
            idx += 1
            values = partition["Values"]
            if idx >= 25:
                glue.delete_partitions(db_name, table_name, partitions_to_delete)
                partitions_to_delete = []
                idx = 0            
            partitions_to_delete.append({ "Values": values})                    
        if token is None or len(token) == 0:
            break    
    response = glue.delete_partitions(db_name, table_name, partitions_to_delete)



    



def cli(context, args):
    util.set_logger(args.verbose)

    from resource_manager_common import constant
    credentials = context.aws.load_credentials()

    resources = util.get_resources(context)
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]              
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.ENV_S3_STORAGE] = resources[c.RES_S3_STORAGE]  
    os.environ[c.ENV_DEPLOYMENT_STACK_ARN] = resources[c.ENV_STACK_ID]
    os.environ["AWS_LAMBDA_FUNCTION_NAME"] = resources[c.RES_S3_STORAGE]
    os.environ["AWS_ACCESS_KEY"] = args.aws_access_key if args.aws_access_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.ACCESS_KEY_OPTION)
    os.environ["AWS_SECRET_KEY"] = args.aws_secret_key if args.aws_secret_key else credentials.get(args.profile if args.profile else context.config.user_default_profile, constant.SECRET_KEY_OPTION)
    main({c.ENV_STACK_ID:resources[c.ENV_STACK_ID]}, type('obj', (object,), {'function_name' : resources[c.RES_DB_TABLE_CONTEXT]}))    


