import boto3
from multiprocessing.dummy import Pool
from botocore.client import Config

def run_job(job):
    return job.run()

class TransferJobManager(object):
    def __init__(self, pool):
        self.pool = pool
        self.results = []
        self.responses = []

    def add_jobs(self, jobs):
        self.responses.append(self.pool.map_async(run_job, jobs))

    def close_pool(self):
        self.pool.close()

    def wait_for_finish(self):
        self.pool.join()

    def concatenate_results(self):
        return [ response.get() for response in self.responses ]

def CreateTransferJob(response_content, source_bucket, target_bucket):
    return s3TransferJob(response_content, source_bucket, target_bucket)

class s3TransferJob(object):
    def __init__(self, response_content, source_bucket, target_bucket):
        self.client = boto3.client('s3', config=Config(signature_version='s3v4'))
        self.description = {
            "TargetBucket": target_bucket,
            "SourceBucket": source_bucket
        }
        for k,v in response_content.iteritems():
            self.description[k] = v

    def get_decription(self):
        return self.description

    def run(self):
        job_response = { "Key": self.description["Key"] }

        try:
            self.client.copy({
                    'Bucket': self.description["SourceBucket"],
                    'Key': self.description["Key"]
                },
                self.description["TargetBucket"],
                Key=self.description["Key"])
        except Exception as e:
            job_response["TransferException"] = e

        return job_response
