import metric_schema as schema
#context keys
KEY_PRIMARY = "key"
KEY_SECONDARY ="value"
KEY_SEPERATOR_CSV = "csv_seperator"
KEY_SEPERATOR_PARTITION = "partition_seperator"
KEY_CSV_PARQUET_COMPRESSION_RATIO = "csv_parquet_compression_ratio"
KEY_TARGET_AGGREGATION_FILE_SIZE_IN_MB = "amoeba_target_aggregation_file_size_in_MB"
KEY_PARTITIONS = "partitions"
KEY_FILTERS = "filters"
KEY_PRIORITIES = "priorities"
KEY_AGGREGATION_PERIOD_IN_SEC = "aggregation_period_in_sec"
KEY_METRIC_BUCKET = "metric_bucket"
KEY_RECORD_DETAIL_METRIC_DATA = "record_detailed_metric_data"
KEY_MSG_IDS = "msg_ids"
KEY_APPENDER = "append"
KEY_SQS_CLIENT = "sqs_client"
KEY_ATHENA_QUERY = "query_client"
KEY_GLUE_CRAWLER = "glue_client"
KEY_SUCCEEDED_MSG_IDS = "succeeeded_msg_ids"
KEY_SET = "set"
KEY_SET_COLUMNS = "columns"
KEY_START_TIME = "start_time"
KEY_TABLES = "tables"
KEY_BACKOFF_BASE_SECONDS = "backoff_base_seconds"
KEY_BACKOFF_MAX_SECONDS = "backoff_max_seconds"
KEY_BACKOFF_MAX_TRYS = "backoff_max_trys"
KEY_MAX_LAMBDA_TIME = "max_lambda_execution_time"
KEY_STACK_PREFIX = "stack_prefix"
KEY_DB = "db"
KEY_SQS = "sqs"
KEY_SQS_AMOEBA = "sqs_amoeba"
KEY_SQS_AMOEBA_SUFFIX = "amoeba"
KEY_S3 = "s3"
KEY_LAMBDA = "lambda"
KEY_THREAD_POOL = "threadpool"
KEY_CLOUDWATCH = "cloudwatch"
KEY_AGGREGATOR = "agg"
KEY_MAX_MESSAGE_RETRY = "max_message_retry"
KEY_VERBOSE = "verbose"
KEY_GROWTH_RATE_BEFORE_ADDING_LAMBDAS = "growth_rate_trigger"
KEY_FIFO_GROWTH_TRIGGER = "fifo_limit_before_new_q"
KEY_MEMORY_FLUSH_TRIGGER = "mem_trigger"
KEY_AMOEBA_MEMORY_FLUSH_TRIGGER = "amoeba_mem_trigger"
KEY_IS_LAMBDA_ENV = "is_lambda"
KEY_LAMBDA_FUNCTION = "lambda_func"
KEY_REQUEST_ID = "request_id"
KEY_SQS_QUEUE_URL = "sqs_url"
KEY_NUMBER_OF_INIT_LAMBDAS = "number_of_initial_consumer_invokes"
KEY_THRESHOLD_BEFORE_SPAWN_NEW_CONSUMER = "threshold_before_spawning_new_lambda"
KEY_MAX_INFLIGHT_MESSAGES = "max_inflight_messages"
KEY_FREQUENCY_TO_CHECK_TO_SPAWN_ANOTHER = "frequency_to_check_to_spawn"
KEY_FREQUENCY_TO_CHECK_SQS_STATE = "frequency_to_check_sqs_state"
KEY_HEATMAPS = "heatmaps"
KEY_SAVE_GLOBAL_COORDINATES = "write_long_lat"
KEY_WRITE_DETAILED_CLOUDWATCH_EVENTS = "write_detailed_cloudwatch_event"

FILE_PARTIAL_PREFIX = "__partial"
NEW_LINE = "\n"
FILENAME_SEP = "_"
CSV_SEP_DEFAULT = ";"
ONE_GB_IN_BYTES = 1073741824
SENSITIVE = 'sensitive'
INSENSITIVE = 'insensitive'
KMS_KEY_ID = "S3KMSKEY"
PREDEFINED_PARTITIONS = [schema.Required.source().id,
                        schema.Required.build_id().id,
                        schema.Required.Server.server_timestamp_utc().id,
                        schema.Required.event().id,
                        schema.Required.RequestParameters.sensitivity().id,
                        schema.Required.RequestParameters.schema_hash().id]
NON_SETTINGS = [KEY_PARTITIONS, KEY_FILTERS, KEY_PRIORITIES, KEY_SEPERATOR_PARTITION]
PRIMARY_KEY = schema.Required.Server.uuid().long_name
SECONDARY_KEY = schema.Required.event().long_name
TERTIARY_KEY = schema.Required.Server.server_timestamp_utc().long_name
IS_LOCALLY_RUN = "local_run"

#GAME CLIENT
CLIENT_BUFFER_FLUSH_TO_FILE_IN_BYTES = "buffer_flush_to_file_max_in_bytes"
CLIENT_BUFFER_GAME_FLUSH_PERIOD_IN_SEC = "buffer_flush_to_file_interval_sec"
CLIENT_FILE_NUM_METRICS_TO_SEND_IN_BATCH = "file_max_size_in_mb"
CLIENT_FILE_SEND_METRICS_INTERVAL_IN_SECONDS = "file_send_metrics_interval_in_seconds"
CLIENT_FILE_MAX_METRICS_TO_SEND_IN_BATCH_IN_MB = "file_max_metrics_to_send_in_batch_in_mb"
CLIENT_FILE_PRIORITIZATION_THRESHOLD_IN_PERC = "file_threshold_to_prioritize_in_perc"

#AGGREGATION INFO
INFO_BYTES = "bytes"
INFO_ROWS = "rows"
INFO_TOTAL_BYTES = "total_bytes_uncompressed"
INFO_TOTAL_ROWS = "total_rows"
INFO_TOTAL_MESSAGES = "total_messages"
INFO_EVENTS = "events"

#CLOUDWATCH
CW_ATTR_SAVE_DURATION = "save_duration_seconds"
CW_ATTR_DELETE_DURATION = "delete_duration"
CW_METRIC_DIMENSION_SAVE = 'S3AverageWriteDuration'
CW_METRIC_DIMENSION_DEL = 'FIFODeleteDuration'
CW_METRIC_DIMENSION_MSG = 'FIFOMessages'
CW_METRIC_DIMENSION_BYTES = "UncompressedBytes"
CW_METRIC_DIMENSION_ROWS = "Metrics"
CW_METRIC_NAMESPACE = "CloudCanvas"
CW_METRIC_NAME_PROCESSED = "Processed"
CW_METRIC_DIMENSION_NAME_CONSUMER = "Consumer"
CW_MAX_METRIC_SUBMISSIONS = 20

#SQS
RATIO_OF_MAX_LAMBDA_TIME = 0.8

#S3 
SAVE_WINDOW_IN_SECONDS = 45


#SQS message maximum is 262144, but Lambda is 131072
MAXIMUM_MESSAGE_SIZE_IN_BYTES = 262144
SQS_PARAM_COMPRESSION_TYPE = "compression_mode"
SQS_PARAM_SENSITIVITY_TYPE = "sensitivity_type"
SQS_PARAM_PAYLOAD_TYPE = "payload_type"
SQS_PARAM_MESSAGE_GROUP_ID = "MessageGroupId"
SQS_PARAM_QUEUE_URL = "QueueUrl"
SQS_PARAM_MESSAGE_DEPULICATIONID = "MessageDeduplicationId"
SQS_PARAM_DELAY_SECONDS = "DelaySeconds"
SQS_PARAM_MESSAGE_ATTRIBUTES = "MessageAttributes"

#SERVICEAPI
API_PARAM_PAYLOAD = "payload"
API_PARAM_SOURCE_IP = "sourceIp"
API_PARAM_DATA = "data"

#INTERFACE
INT_METRICS_LISTENER = "MetricsListener"

#DASHBOARD
DASHBOARD_TELEMETRY =  "telemetry"
DASHBOARD_ENGAGEMENT = "engagement"

#LAMBDA
MAXIMUM_PAYLOAD_SIZE = 131072    
MAXIMUM_ASYNC_PAYLOAD_SIZE = 115000

#environment
ENV_DB_TABLE_CONTEXT = "MetricContext"
ENV_S3_STORAGE = "MetricStorage"
ENV_LAMBDA_CONSUMER = "FIFOConsumer"
ENV_LAMBDA_PRODUCER = "FIFOProducer"
ENV_EVENT_EMITTER = "EventEmitter"
ENV_LAMBDA_CONSUMER_LAUNCHER = "LambdaConsumerLauncher"
ENV_AMOEBA_LAUNCHER = "AmoebaLauncher"
ENV_AMOEBA = "Amoeba"
ENV_AMOEBA_PREFIX = "Amoeba"
ENV_VERBOSE = "Verbose"
ENV_REGION = "AWS_REGION"
ENV_SERVICE_ROLE = "GlueCrawlerRole"
ENV_SQS = "SQS"
ENV_STACK_ID = "StackId"
ENV_AMOEBA_LAUNCHER_RULE = "AmoebaLauncherSchedulerRule"
ENV_LAMBDA_LAUNCHER_RULE = "LambdaConsumerLauncherSchedulerRule"
ENV_GLUE_CRAWLER_LAUNCHER = "GlueCrawlerLauncher"
ENV_DEPLOYMENT_STACK_ARN = "DeploymentStackArn"
ENV_SHARED_BUCKET = "ProjectConfigurationBucket"

#resources
RES_DB_TABLE_CONTEXT = ENV_DB_TABLE_CONTEXT
RES_LAMBDA_FIFOCONSUMER  = ENV_LAMBDA_CONSUMER
RES_LAMBDA_FIFOPRODUCER  = ENV_LAMBDA_PRODUCER
RES_S3_STORAGE = ENV_S3_STORAGE
RES_EVENT_EMITTER = ENV_EVENT_EMITTER
RES_SERVICE_ROLE = ENV_SERVICE_ROLE
RES_SQS = ENV_SQS
RES_AMOEBA = ENV_AMOEBA
RES_GLUE_CRAWLER_LAUNCHER = ENV_GLUE_CRAWLER_LAUNCHER
RES_SHARED_BUCKET = "Configuration"
RES_GEM_NAME = "CloudGemMetric"


