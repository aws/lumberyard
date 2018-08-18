
import service
import os
import metric_constant as c
import errors
import datetime
import util
import metric_schema as schema
from dynamodb import DynamoDb
from cloudwatch import CloudWatch
from athena import Athena, Query, DEFAULT_EVENTS
from aws_lambda import Lambda
from cgf_utils import custom_resource_response
from glue import Glue
import athena

@service.api
def duplication_rate(request):  
    query = Query(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])     
    results = query.execute_with_format('''
          with source as (
              select '''+ schema.SERVER_TIMESTAMP.long_name +''' as srv_tmutc,
                '''+ schema.UUID.long_name +''' as uuid
              from
                "{0}"."{1}''' + DEFAULT_EVENTS.SESSIONSTART + '''"
              WHERE p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime > date_format((current_timestamp - interval '24' hour), '%Y%m%d%H0000')
              UNION
              select '''+ schema.SERVER_TIMESTAMP.long_name +''' as srv_tmutc,
                '''+ schema.UUID.long_name +''' as uuid
              from
                "{0}"."{1}''' + DEFAULT_EVENTS.CLIENTINITCOMPLETE + '''"
              WHERE p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime > date_format((current_timestamp - interval '24' hour), '%Y%m%d%H0000')
          )

            SELECT to_unixtime(from_iso8601_timestamp(T1.tmp)) AS Timestmp,
                     round((T1.value1 - T1.value2) / (T1.value2 * 1.0), 6) AS DuplicationRate
            FROM 
                (SELECT date_format(from_unixtime(srv_tmutc),
                    '%Y-%m-%dT%H:00:00Z') AS tmp, count(uuid) AS value1, count(distinct uuid) AS value2
                FROM source    
                GROUP BY  1) AS T1
            ORDER BY  1 asc''')
    return convert_to_tuple_dataset(results)

@service.api
def events(request):  
    glue = Glue()    
    return glue.get_events()

@service.api
def platforms(request):  
    query = Query(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])     
    results = query.execute_with_format("select distinct T3.plt " \
        "from " \
        "( " \
        "SELECT distinct "+ schema.PLATFORM_ID.long_name +" as plt FROM \"{0}\".\"{1}" + DEFAULT_EVENTS.CLIENTINITCOMPLETE +"\" as T1 " \
        ") as T3 order by 1 asc ")    
    return convert_to_dataset(results)

@service.api
def run_crawler(request):
    context=dict({})
    lb = Lambda(context)
    response = lb.invoke(os.environ[c.ENV_GLUE_CRAWLER_LAUNCHER])    
    return custom_resource_response.success_response({}, "*")

@service.api
def get_crawler_status(request, name):
    glue_crawler = Glue()    
    response = glue_crawler.get_crawler(name)    
    return custom_resource_response.success_response({ "State": response['Crawler']['State']}, "*")

@service.api
def query(request, sql):    
    sql = sql["sql"]
    query = Query(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])         
    return query.execute(sql, sync=False)

@service.api
def query_results(request, id):        
    query = Query(os.environ[c.ENV_DEPLOYMENT_STACK_ARN])       
    results = query.client.get_query_execution(id)  
    #the JSON serializer doesn't support these types right now    
    del results['Status']['SubmissionDateTime']
    if 'CompletionDateTime' in results['Status']:
        del results['Status']['CompletionDateTime']
    if results['Status']['State'] == 'SUCCEEDED':
        results['Result'] = query.client.get_output( results['ResultConfiguration']['OutputLocation'] )
    return results
  
def convert_to_tuple_dataset(data):
    if data == None or len(data) == 0:
        return data
    del data[0]    
    dataset = []
    for row in data:        
        dataset.append({
                'Value': float(row[1]),
                'Timestamp': float(row[0]),
                'Unit': 'Count'
            })
    return dataset

def convert_to_dataset(data):
    if data == None or len(data) == 0:
        return data
    del data[0]    
    dataset = []
    for row in data:        
        dataset.append(row[0])
    return dataset
        

def cli(context, args):
    #this import is only available when you execute via cli
    import util
    resources = util.get_resources(context)        
    os.environ[c.ENV_DB_TABLE_CONTEXT] = resources[c.RES_DB_TABLE_CONTEXT]   
    os.environ[c.ENV_LAMBDA_CONSUMER] = resources[c.RES_LAMBDA_FIFOCONSUMER]   
    os.environ[c.ENV_LAMBDA_PRODUCER] = resources[c.RES_LAMBDA_FIFOPRODUCER]   
    os.environ[c.ENV_AMOEBA_1] = resources[c.RES_AMOEBA_1]   
    os.environ[c.ENV_VERBOSE] = str(args.verbose) if args.verbose else ""
    os.environ[c.ENV_REGION] = context.config.project_region
    os.environ[c.ENV_S3_STORAGE] = resources[c.RES_S3_STORAGE]  
    os.environ["AWS_LAMBDA_FUNCTION_NAME"] = os.environ[c.ENV_LAMBDA_PRODUCER]
    
    print query( type('obj', (object,), {c.ENV_STACK_ID: resources[c.ENV_STACK_ID]}), {"sql":args.sql})
