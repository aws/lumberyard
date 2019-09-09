import os
import boto3
from botocore.exceptions import ClientError

class CloudWatch(object):

    def __init__(self):        
        self.__client = boto3.client('cloudwatch', region_name=os.environ.get('AWS_REGION'), api_version='2010-08-01')        
   
    def put_metric_data(self, namespace, metric_data):                
        try:            
            return self.__client.put_metric_data(Namespace=namespace, MetricData=metric_data)                 
        except ClientError as e:
            print e            
        return 
        
