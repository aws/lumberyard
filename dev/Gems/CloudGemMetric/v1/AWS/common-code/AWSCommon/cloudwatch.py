import retry
import boto3
import metric_constant as c
import time
import os
import json
import datetime
import math
import time 

class CloudWatch(object):

    def __init__(self, context={}):        
        self.__context = context        
        self.__client = boto3.client('cloudwatch',region_name=os.environ[c.ENV_REGION], api_version='2010-08-01') 
        self.__avg_save_duration = None;    
        self.__avg_delete_duration = None;
        self.__sum_save_duration = None;    
        self.__sum_delete_duration = None;

    def get_metric(self, namespace, metric_name, dimension_name, dimension_value, stat, start = None, end = None, period=300):
        start = datetime.datetime.utcnow() - datetime.timedelta(days=4) if not start else start
        end = datetime.datetime.utcnow() if not end else end   
        start = datetime.datetime(start.year, start.month, start.day, start.hour, start.minute)
        end = datetime.datetime(end.year, end.month, end.day, end.hour, end.minute)
        data=self.__client.get_metric_statistics(
            Namespace=namespace,
            MetricName=metric_name,
            Dimensions=[{ 'Name': dimension_name, 'Value': dimension_value}],
            StartTime=datetime.datetime(start.year, start.month, start.day, start.hour, start.minute),
            EndTime=datetime.datetime(end.year, end.month, end.day, end.hour, end.minute),
            Statistics=[stat],
            Period=period
        )
        if 'Datapoints' not in data:
            return []
        result = []
        for point in data['Datapoints']:            
            point['Timestamp']=time.mktime(point['Timestamp'].timetuple())
            result.append(point)        
        return result

    def put_metric_data(self, namespace, metric_data):
        #print metric_data        
        return self.__client.put_metric_data(
            Namespace=namespace,
            MetricData=metric_data
        )
        
    def avg_save_duration(self, stack_id):
        if self.__avg_save_duration != None: 
            return self.__avg_save_duration

        self.__avg_save_duration = self.find_metrics_avg(c.CW_METRIC_DIMENSION_SAVE, stack_id)  
        return self.__avg_save_duration
    
    def avg_delete_duration(self, stack_id):
        if self.__avg_delete_duration != None: 
            return self.__avg_delete_duration        
        self.__avg_delete_duration = self.find_metrics_avg(c.CW_METRIC_DIMENSION_DEL, stack_id)  
        return self.__avg_delete_duration

    def sum_save_duration(self, stack_id):
        if self.__sum_save_duration != None: 
            return self.__sum_save_duration

        self.__sum_save_duration = self.find_metrics_sum(c.CW_METRIC_DIMENSION_SAVE, stack_id)  
        return self.__sum_save_duration
    
    def sum_delete_duration(self, stack_id):
        if self.__sum_delete_duration != None: 
            return self.__sum_delete_duration        
        self.__sum_delete_duration = self.find_metrics_sum(c.CW_METRIC_DIMENSION_DEL, stack_id)  
        return self.__sum_delete_duration

    def find_metrics_avg(self, dimension, stack_id):
        max_retries = 5
        attempt = 0
        avg = 0
        minutes = 10
        while attempt < max_retries:            
            data = self.get_metric(stack_id, c.CW_METRIC_NAME_PROCESSED,  c.CW_METRIC_DIMENSION_NAME_CONSUMER, dimension, "Average", start=(datetime.datetime.today() - datetime.timedelta(minutes=minutes)))                    
            if len(data) > 0:                
                for point in data:                        
                    avg += point['Average']
                avg = avg /  len(data) 
                break           
            else:
                minutes += 20
                attempt += 1
        return math.ceil(avg)

    def find_metrics_sum(self, dimension, stack_id):
        max_retries = 5
        attempt = 0
        sum = 0
        minutes = 1
        while attempt < max_retries:            
            data = self.get_metric(stack_id, c.CW_METRIC_NAME_PROCESSED,  c.CW_METRIC_DIMENSION_NAME_CONSUMER, dimension, "Sum", start=(datetime.datetime.today() - datetime.timedelta(minutes=minutes)))                    
            if len(data) > 0:                
                for point in data:                        
                    sum += point['Sum']
                break           
            else:
                minutes += minutes
                attempt += 1
        return math.ceil(sum)
