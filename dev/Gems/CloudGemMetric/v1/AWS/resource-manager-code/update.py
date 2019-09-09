#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
import sys
sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/Constant')
sys.path.append('./Gems/CloudGemMetric/v1/AWS/common-code/MetricUtils')
import random
import string
import metric_constant as c
import metric_schema as schema
import util
import uuid
from botocore.client import Config
from botocore.exceptions import ClientError

def after_this_resource_group_updated(hook, deployment_name, **kwargs):
    context = hook.context
    dynamoDB = context.aws.session.client('dynamodb', region_name=context.config.project_region)
    gem_name = kwargs['resource_group_name']
    deployment_arn = context.config.get_deployment_stack_id(deployment_name)
    gem_resource_group = context.stack.describe_resources(deployment_arn, recursive=True)
    db_id = gem_resource_group[gem_name+'.MetricContext']['PhysicalResourceId']
    response = dynamoDB.scan(
        TableName=db_id
    )

    if response and len(response['Items']) > 0:
        return
    params = dict({})

    params["RequestItems"]={
        db_id:
        [
            {
                "PutRequest":
                {
                    "Item": {
                        "key": { "S": c.KEY_GROWTH_RATE_BEFORE_ADDING_LAMBDAS },
                        "value": { "N": "0.05" }
                    }
                }
            },
            {
                "PutRequest":        
                {
                    "Item": {
                        "key": { "S": c.KEY_CSV_PARQUET_COMPRESSION_RATIO },
                        "value": { "N": "13" }
                    }
                }
            },
            {
                "PutRequest":        
                {
                    "Item": {
                        "key": { "S": c.KEY_FREQUENCY_TO_CHECK_TO_SPAWN_ANOTHER },
                        "value": { "N": "5" }
                    }
                },
            },
            {
                "PutRequest":        
                {    
                    "Item": {
                        "key": { "S": c.KEY_FREQUENCY_TO_CHECK_SQS_STATE },
                        "value": { "N": "5" }
                    }
                }
            },
            {
                "PutRequest":        
                {    
                    "Item": {
                        "key": { "S": c.KEY_BACKOFF_MAX_SECONDS },
                        "value": { "N": "5.0" }
                    }
                }
            },
            {
                "PutRequest":        
                {    
                    "Item": {
                        "key": { "S": c.KEY_BACKOFF_BASE_SECONDS },
                        "value": { "N": "5.0" }
                    }
                },
            },
            {
                "PutRequest":        
                {    
                    "Item": {
                        "key": { "S": c.KEY_RECORD_DETAIL_METRIC_DATA },
                        "value": { "BOOL": False }
                    }
                }
            },    
            {
                "PutRequest":        
                {   
                    "Item": { 
                        "key": { "S": c.KEY_BACKOFF_MAX_TRYS },
                        "value":  { "N": "5.0" }
                    }
                }    
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_MAX_MESSAGE_RETRY },
                        "value":  { "N": "10" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_SEPERATOR_CSV },
                        "value":  { "S": c.CSV_SEP_DEFAULT }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_FIFO_GROWTH_TRIGGER },
                        "value":  { "N": "3000" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_MEMORY_FLUSH_TRIGGER },
                        "value":  { "N": "75" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_AMOEBA_MEMORY_FLUSH_TRIGGER },
                        "value":  { "N": "60" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_TARGET_AGGREGATION_FILE_SIZE_IN_MB },
                        "value":  { "N": "32" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_MAX_LAMBDA_TIME },
                        "value":  { "N": "275" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_SEPERATOR_PARTITION },
                        "value":  { "S": "/" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_AGGREGATION_PERIOD_IN_SEC },
                        "value":  { "N": "220" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_THRESHOLD_BEFORE_SPAWN_NEW_CONSUMER },
                        "value":  { "N": "3000" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_MAX_INFLIGHT_MESSAGES },
                        "value":  { "N": "12000" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_FILTERS },
                        "value":  { "L": [] }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_PRIORITIES },
                        "value":  { "L": [] }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_NUMBER_OF_INIT_LAMBDAS },
                        "value":  { "N": "3" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.CLIENT_BUFFER_FLUSH_TO_FILE_IN_BYTES },
                        "value":  { "N": "204800" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.CLIENT_BUFFER_GAME_FLUSH_PERIOD_IN_SEC },
                        "value":  { "N": "60" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.CLIENT_FILE_NUM_METRICS_TO_SEND_IN_BATCH },
                        "value":  { "N": "5" }
                    }
                }
            }
        ]   
    }
    #batch write has a 25 length limit    
    #hard coding the schema.SERVER_TIMESTAMP.id datetime format to '%Y%m%d%H0000' as there seems to be an import issues for the util module that occassionally fails the tests.
    dynamoDB.batch_write_item(**params)
    params["RequestItems"]={
        db_id:        
        [
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.CLIENT_FILE_SEND_METRICS_INTERVAL_IN_SECONDS },
                        "value":  { "N": "300" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.CLIENT_FILE_MAX_METRICS_TO_SEND_IN_BATCH_IN_MB },
                        "value":  { "N": "5" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.CLIENT_FILE_PRIORITIZATION_THRESHOLD_IN_PERC },
                        "value":  { "N": "60" }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_HEATMAPS },
                        "value": { "L": [] }
                    }
                }
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_SAVE_GLOBAL_COORDINATES },
                        "value": { "BOOL": False }
                    }
                }    
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_WRITE_DETAILED_CLOUDWATCH_EVENTS },
                        "value": { "BOOL": False }
                    }
                }    
            },
            {
                "PutRequest":        
                {   
                    "Item": {
                        "key": { "S": c.KEY_PARTITIONS },
                        "value":  { "L": [
                                {"M":{
                                  "key": { "S": schema.EVENT.id },
                                  "parts": { "L": [] } ,
                                  "type": {"S":"str"},
                                  "description": {"S":"The identifying name of the game event."}
                                }},
                                {"M":{
                                  "key": { "S": schema.SERVER_TIMESTAMP.id },
                                  "parts": { "L": [
                                    {"S":".strftime('%Y%m%d%H0000')"}
                                  ]},
                                  "type": {"S":"datetime.datetime.utcfromtimestamp"},
                                  "description": {"S":"The server UTC timestamp.  This partition has parts associated to it.  The parts will result in a S3 folder gather data in a folder named 20180101230000"}
                                }},
                                {"M":{
                                  "key": { "S": schema.SERVER_TIMESTAMP.id },
                                  "parts": { "L": [
                                    {"S":".year"},
                                    {"S":".month"},
                                    {"S":".day"},
                                    {"S":".hour"}
                                  ]},
                                  "type": {"S":"datetime.datetime.utcfromtimestamp"},
                                  "description": {"S":"The server UTC timestamp.  This partition has parts associated to it.  The parts will be extracted from the srv_tmutc attribute value.  Example:  2018-01-01T13:00:00Z will result in a S3 key path .../2018/01/01/05/..."}
                                }},
                                {"M":{
                                  "key": { "S": schema.SOURCE.id },
                                  "parts": { "L": [] },
                                  "type": {"S":"str"},
                                  "description": {"S":"Where the data is originating from.  Most often this will be 'cloudgemmetric' but could be other Cloud Gems such as Cloud Gem Defect Reporter."}
                                }},
                                {"M":{
                                  "key": { "S": schema.BUILD_ID.id },
                                  "parts": { "L": [] },
                                  "type": {"S":"str"},
                                  "description": {"S":"The build identifier the event originated on."}
                                }},
                                {"M":{
                                  "key": { "S": schema.SENSITIVITY.id },
                                  "parts": { "L": [] },
                                  "type": {"S":"map"},
                                  "description": {"S":"A flag used to defining if the data is encrypted in S3."}
                                }},
                                {"M":{
                                  "key": { "S": schema.SCHEMA_HASH.id },
                                  "parts": { "L": [] },
                                  "type": {"S":"map"},
                                  "description": {"S":"A hash of the event schema."}
                                }}
                              ]
                        }
                    }
                }
            }
        ]
    }
    dynamoDB.batch_write_item(**params)
    seed_cgp_queries(db_id, dynamoDB)


def seed_cgp_queries(db_id, dynamoDB):
    params = dict({})
    params["TableName"]=db_id
    params["Item"]={
        "key": { "S": c.DASHBOARD_ENGAGEMENT },
        "value":  { "L": [
                    {"M":{
                      "dataserieslabels": { "SS": ["Players"] },
                      "parsers": { "SS": ["barData"] },
                      "sql": { "SS": [
                            '''SELECT date_format(from_unixtime('''+ schema.SERVER_TIMESTAMP.long_name +'''), '%Y-%m-%d') as Day, count(distinct '''+ schema.UID.long_name +''') AS count FROM ${database}.${table_sessionstart} WHERE p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime >= date_format((current_timestamp - interval '31' day), '%Y%m%d%H0000') GROUP BY  1 order by 1 asc'''
                            ] },
                      "title": {"S":"Daily Active Users (DAU)"},
                      "type": {"S":"ngx-charts-bar-vertical"},
                      "xaxislabel": {"S":"Day"},
                      "yaxislabel": {"S":"Distinct Player Count"},
                      "id": {"S": str(uuid.uuid1())}
                    }},
                    {"M":{
                      "dataserieslabels": { "SS": ["Players"] },
                      "parsers": { "SS": [ "barData" ] },
                      "sql": { "SS": [
                            '''SELECT date_format(from_unixtime('''+ schema.SERVER_TIMESTAMP.long_name +'''), '%Y-%m') as Month, count(distinct '''+ schema.UID.long_name +''') AS count FROM ${database}.${table_sessionstart} WHERE p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime >= date_format((current_timestamp - interval '12' month), '%Y%m%d%H0000') GROUP BY  1 order by 1 asc'''
                            ] },
                      "title": {"S":"Monthly Active Users (MAU)"},
                      "type": {"S":"ngx-charts-bar-vertical"},
                      "xaxislabel": {"S":"Month"},
                      "yaxislabel": {"S":"Distinct Player Count"},
                      "id": {"S": str(uuid.uuid1())}
                    }},
                    {"M":{
                      "dataserieslabels": { "SS": ["Play Length"] },
                      "parsers": { "SS": [ "barData" ] },
                      "sql": { "SS": [
                            '''
                            with source as ( select distinct srv_tmutc as event_timestamp, client_cognito_id as client_cognito_id, session_id 
                                             from   <ALL_TABLES_UNION>(select '''+ schema.SERVER_TIMESTAMP.long_name +''' as srv_tmutc, '''+ schema.UID.long_name +''' as client_cognito_id, '''+ schema.SESSION_ID.long_name +''' as session_id
                                             from ${database}.${table}
                                             where (p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_day = cast(day(current_timestamp - interval '1' day) as varchar)
                                             and p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_month = cast(month(current_timestamp - interval '1' day) as varchar)
                                             and p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_year = cast(year(current_timestamp - interval '1' day) as varchar))
                                             OR (p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_day = cast(day(current_timestamp - interval '7' day) as varchar)
                                             and p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_month = cast(month(current_timestamp - interval '7' day) as varchar)
                                             and p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_year = cast(year(current_timestamp - interval '7' day) as varchar))
                                             OR (p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_day = cast(day(current_timestamp - interval '30' day) as varchar)
                                             and p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_month = cast(month(current_timestamp - interval '30' day) as varchar)
                                             and p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_year = cast(year(current_timestamp - interval '30' day) as varchar))                                                
                                             )</ALL_TABLES_UNION>
                                             ),
                            useractivity as ( select date_format(from_unixtime(event_timestamp), '%Y-%m-%d') as event_timestamp,
                                              client_cognito_id, session_id, max(event_timestamp) as max, min(event_timestamp) as min
                                              from source gt 
                                              group by 1, 2, 3  )
                            select case 
                                when event_timestamp = date_format(current_timestamp - interval '1' day, '%Y-%m-%d') then 'D1' 
                                when event_timestamp = date_format(current_timestamp - interval '7' day, '%Y-%m-%d') then 'D7' 
                                when event_timestamp = date_format(current_timestamp - interval '30' day, '%Y-%m-%d') then 'D30' 
                                end, round(avg(play_time_per_session_in_seconds), 4) as avg_session_time_in_seconds_per_day
                            from (select event_timestamp, client_cognito_id, (ua.max - ua.min) / 1000 as play_time_per_session_in_seconds 
                                  from useractivity ua)
                            group by 1
                            order by 1
                            '''
                            ] },
                      "title": {"S":"Average Play Time"},
                      "type": {"S":"ngx-charts-bar-vertical"},
                      "xaxislabel": {"S":"Day"},
                      "yaxislabel": {"S":"Duration (s)"},
                      "id": {"S": str(uuid.uuid1())}
                    }},
                    {"M":{
                      "dataserieslabels": { "SS": [ "Players" ] },
                      "colorscheme": { "M": {
                            "domain": { "SS":  [
                              "#551A8B",
                              "#981377",
                              "#9AC418",
                              "#CEC51A",
                              "#C6192D",
                              "#126E7F"                            
                            ]
                        }}
                      },
                      "parsers": { "SS": ["lineData"] },
                      "sql": { "SS": [
                            '''with useractivity as ( select distinct date(date_format(from_unixtime('''+ schema.SERVER_TIMESTAMP.long_name +'''), '%Y-%m-%d')) as event_timestamp, '''+ schema.UID.long_name +''' as client_cognito_id
                                                from ${database}.${table_sessionstart} gt
                                                where p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime >= date_format((current_timestamp - interval '30' day), '%Y%m%d%H0000')
                                                and p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime < date_format(current_date, '%Y%m%d000000')),                            
                                retention_t as ( select cur.event_timestamp,
                                                    count(cur.client_cognito_id) as active_users,
                                                    count(fut1.client_cognito_id) as one_day_retained_users,
                                                    count(fut1.client_cognito_id) / (count(cur.client_cognito_id) * 1.0) as one_day_retention,
                                                    count(fut7.client_cognito_id) as seven_day_retained_users,
                                                    count(fut7.client_cognito_id) / (count(cur.client_cognito_id) * 1.0) as seven_day_retention,
                                                    count(fut30.client_cognito_id) as thirty_day_retained_users,
                                                    count(fut30.client_cognito_id) / (count(cur.client_cognito_id) * 1.0) as thirty_day_retention
                                                from useractivity cur left join useractivity fut1
                                                    on cur.client_cognito_id = fut1.client_cognito_id
                                                    and cur.event_timestamp = (fut1.event_timestamp - interval '1' day) left join useractivity fut7
                                                    on cur.client_cognito_id = fut7.client_cognito_id
                                                    and cur.event_timestamp = (fut7.event_timestamp - interval '7' day) left join useractivity fut30
                                                    on cur.client_cognito_id = fut30.client_cognito_id
                                                    and cur.event_timestamp = (fut30.event_timestamp - interval '30' day)
                                                    group by 1
                                                    )
                                select event_timestamp,
                                    round(avg(one_day_retention) over (order by event_timestamp rows between 6 preceding and 0 following)* 100, 2) as "D1 Avg",
                                    round(avg(seven_day_retention) over (order by event_timestamp rows between 6 preceding and 0 following) * 100, 2) as "D7 Avg",
                                    round(avg(thirty_day_retention) over (order by event_timestamp rows between 6 preceding and 0 following)* 100, 2) as "D30 Avg"
                                from retention_t where event_timestamp >= (current_timestamp - interval '7' day) order by 1 desc'''] },
                      "title": {"S":"Retention"},
                      "type": {"S":"ngx-charts-line-chart"},
                      "xaxislabel": {"S":"Day"},
                      "yaxislabel": {"S":"Percentage"},
                      "id": {"S": str(uuid.uuid1())}
                    }}
                  ]
            }
    }
    dynamoDB.put_item(**params)
    params["Item"]={
        "key": { "S": c.DASHBOARD_TELEMETRY },
        "value":  { "L": [
                    {"M":{
                      "dataserieslabels": { "SS": ["Players"] },
                      "parsers": { "SS": ["barData"] },
                      "sql": { "SS": [
                            '''SELECT date_format(from_unixtime('''+ schema.SERVER_TIMESTAMP.long_name +'''), '%m-%d %Hh') as Hour, count(distinct '''+ schema.UID.long_name +''') AS count FROM ${database}.${table_sessionstart} WHERE p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime >= date_format((current_timestamp - interval '24' hour), '%Y%m%d%H0000')  GROUP BY 1 order by 1 asc'''
                            ] },
                      "title": {"S":"Active Players For Past 24 Hours"},
                      "type": {"S":"ngx-charts-bar-vertical"},
                      "xaxislabel": {"S":"Date"},
                      "yaxislabel": {"S":"Distinct Player Count"},
                      "id": {"S": str(uuid.uuid1())}
                    }},
                    {"M":{
                      "dataserieslabels": { "SS": ["Platform"] },
                      "parsers": { "SS": ["lineData"] },
                      "sql": { "SS": [
                            '''with source as (SELECT date_format(from_unixtime('''+ schema.SERVER_TIMESTAMP.long_name +'''), '%m-%d %Hh') AS Hour, '''+ schema.PLATFORM_ID.long_name +''' as pltf, count(distinct '''+ schema.UID.long_name +''') AS count FROM ${database}.${table_sessionstart} WHERE p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime >= date_format((current_timestamp - interval '24' hour), '%Y%m%d%H0000') GROUP BY  1, 2 order by 1) select pltf, Hour, count from source where source.pltf is not null order by Hour asc''' ] },
                      "title": {"S":"Active Platform Players For Past 24 Hours"},
                      "type": {"S":"ngx-charts-area-chart-stacked"},
                      "xaxislabel": {"S":"Date"},
                      "yaxislabel": {"S":"Distinct Player Count"},
                      "id": {"S": str(uuid.uuid1())}
                    }},
                    {"M":{
                      "dataserieslabels": { "SS": ["Players"] },
                      "parsers": { "SS": ["lineData"] },
                      "sql": { "SS": [
                            '''SELECT date_format(from_unixtime('''+ schema.SERVER_TIMESTAMP.long_name +'''), '%m-%d %H:%ih') as Hour, count(distinct '''+ schema.UID.long_name +''') AS count FROM ${database}.${table_sessionstart} WHERE p_'''+ schema.SERVER_TIMESTAMP.long_name +'''_strftime >= date_format((current_timestamp - interval '2' hour), '%Y%m%d%H0000')  GROUP BY 1 order by 1 asc'''
                            ] },
                      "title": {"S":"Active Players By The Minute For Past 2 Hours"},
                      "type": {"S":"ngx-charts-line-chart"},
                      "xaxislabel": {"S":"Minutes"},
                      "yaxislabel": {"S":"Distinct Player Count"},
                      "id": {"S": str(uuid.uuid1())}
                    }}
                  ]
            }
    }
    dynamoDB.put_item(**params)



