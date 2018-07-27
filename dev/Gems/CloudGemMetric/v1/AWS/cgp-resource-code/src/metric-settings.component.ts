import { Component, OnInit, Input } from '@angular/core';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { Http } from '@angular/http';
import { AwsService } from 'app/aws/aws.service';
import { CloudGemMetricApi } from './api-handler.class';
import { ToastsManager } from 'ng2-toastr/ng2-toastr';

@Component({
    selector: 'metric-settings',
    template: `
        <div class="metric-settings-container">
            <h2> Configure Settings </h2>
            <ng-container [ngSwitch]="isLoadingSettings">
                <div *ngSwitchCase="true">
                    <loading-spinner></loading-spinner>
                </div>
                <div *ngSwitchCase="false">
                    <form [formGroup]="settingsForm" class="settingsForm" (ngSubmit)="updateSettings()" novalidate>
                        <div class="form-group row">
                            <label class="col-lg-3"> Growth Rate Trigger Percent </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="0" [max]="100" [step]="1.0" [formControl]="settingsForm.controls.growth_rate_trigger_percent" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.growth_rate_trigger_percent.value}}% </span>
                            </div>
                            <tooltip placement="right" tooltip="The SQS message growth rate threshold for when a new consumer will be created."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> CSV Parquet Compression Ratio</label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="50" [step]="1.0" [formControl]="settingsForm.controls.csv_parquet_compression_ratio" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.csv_parquet_compression_ratio.value}}% </span>
                            </div>
                            <tooltip placement="right" tooltip="The estimated compression ratio going from CSV to PARQUET format.  This is used for estimation purposes."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Backoff Max Seconds</label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="15" [step]="1.0" [formControl]="settingsForm.controls.backoff_max_seconds" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.backoff_max_seconds.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The maximum back off period in seconds for failed AWS requests."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Target Amoeba aggregation file size </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="256" [step]="1.0" [formControl]="settingsForm.controls.amoeba_target_aggregation_file_size_in_MB" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.amoeba_target_aggregation_file_size_in_MB.value}} MB </span>
                            </div>
                            <tooltip placement="right" tooltip="The target aggregation file size in MB.  The amoeba file generator will attempt to generate S3 parquet files of this size.  128 MB is ideal."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Number Of Initial Consumers </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="10" [step]="1.0" [formControl]="settingsForm.controls.number_of_initial_consumer_invokes" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.number_of_initial_consumer_invokes.value}} consumer(s) </span>
                            </div>
                            <tooltip placement="right" tooltip="The number of initial SQS consumers to trigger during each schedule execution date.  The consumer lambdas are self-replicating in two circumstances; 1 - The growth rate threshold is exceeded.  2 - The SQS queue size threshold is exceeded."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Backoff Base Seconds</label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="15" [step]="1.0" [formControl]="settingsForm.controls.backoff_base_seconds" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.backoff_base_seconds.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The initial back off period in seconds for failed AWS requests."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Max In-flight Messages</label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1000" [max]="15000" [step]="100" [formControl]="settingsForm.controls.max_inflight_messages" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.max_inflight_messages.value}} messages </span>
                            </div>
                            <tooltip placement="right" tooltip="The maximum allowable inflight messages for any given SQS queue.  If a queue reaches this threshold no more messages will be processed until the inflight number drops below the threshold."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Frequency To Check SQS State</label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="45" [step]="1" [formControl]="settingsForm.controls.frequency_to_check_sqs_state" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.frequency_to_check_sqs_state.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The frequency in which to check the SQS state.  This requires querying the SQS service. 'Frequency To Check Spawning Threshold' should be a multiple of this."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Threshold Before Spawning New Consumer Lamdba</label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1000" [max]="15000" [step]="100" [formControl]="settingsForm.controls.threshold_before_spawning_new_lambda" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.threshold_before_spawning_new_lambda.value}} messages </span>
                            </div>
                            <tooltip placement="right" tooltip="The SQS queue size threshold before a new consumer will be spawned automatically."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Frequency To Check Spawning Threshold</label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="30" [step]="1" [formControl]="settingsForm.controls.frequency_to_check_to_spawn" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.frequency_to_check_to_spawn.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The frequency in which to check the threshold for spawning a new consumer lambda."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Backoff Max Trys </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="15" [step]="1.0" [formControl]="settingsForm.controls.backoff_max_trys" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.backoff_max_trys.value}} attempts </span>
                            </div>
                            <tooltip placement="right" tooltip="The maximum number of attempts for failed AWS requests."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Max Message Retry </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="15" [step]="1.0" [formControl]="settingsForm.controls.max_message_retry" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.max_message_retry.value}} retries </span>
                            </div>
                            <tooltip placement="right" tooltip="The maximum number of retries before a message starting logging as an error.  Messages that are processed multiple times are considered to be in error."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> CSV Seperator </label>
                            <input class="form-control" type="string" formControlName="csv_seperator" >
                            <tooltip placement="right" tooltip="The separator used both for encoding the client CSV and decoding the SQS message payload."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> FIFO Limit Before New Queue </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1000" [max]="15000" [step]="100" [formControl]="settingsForm.controls.fifo_limit_before_new_q" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.fifo_limit_before_new_q.value}} inflight messages </span>
                            </div>
                            <tooltip placement="right" tooltip="The threshold for when a new SQS queue is generated.  The threshold is based on in-flight messages (number of messages being processed) as SQS FIFO queues are limited to 20,000 in-flight messages."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Memory Trigger </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="90" [step]="1.0" [formControl]="settingsForm.controls.mem_trigger" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.mem_trigger.value}}% </span>
                            </div>
                            <tooltip placement="right" tooltip="The memory threshold in percentage for when to start saving to S3 files."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Amoeba Memory Trigger </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="70" [step]="1.0" [formControl]="settingsForm.controls.amoeba_mem_trigger" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.amoeba_mem_trigger.value}}% </span>
                            </div>
                            <tooltip placement="right" tooltip="The memory threshold in percentage for when the amoeba single file generator should to start saving to S3 files."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Max Lambda Execution Time  </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="275" [step]="1.0" [formControl]="settingsForm.controls.max_lambda_execution_time" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.max_lambda_execution_time.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The maximum lambda execution time.  This is used as a internal timer to determine the process windows for each step of the aggregation (parse SQS message, save to S3, delete message)."> </tooltip>
                        </div>                        
                        <div class="form-group row">
                            <label class="col-lg-3"> Aggregation Period </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="1" [max]="275" [step]="1.0" [formControl]="settingsForm.controls.aggregation_period_in_sec" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.aggregation_period_in_sec.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The target aggregation window size.  This will be the default aggregation window size.  It can be overridden based on contextual information provided by telemetry data."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Flush To Local File </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="128" [max]="1048576" [step]="10" [formControl]="settingsForm.controls.buffer_flush_to_file_max_in_bytes" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.buffer_flush_to_file_max_in_bytes.value}} bytes </span>
                            </div>
                            <tooltip placement="right" tooltip="The size in bytes of the memory buffer before sending the metrics in the mermory buffer to local file."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Flush To Local File </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="5" [max]="600" [step]="5" [formControl]="settingsForm.controls.buffer_flush_to_file_interval_sec" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.buffer_flush_to_file_interval_sec.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The period of time in seconds before sending the metrics in the mermory buffer to local file."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Max Local File Size </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="15" [max]="6000" [step]="10.0" [formControl]="settingsForm.controls.file_max_size_in_mb" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.file_max_size_in_mb.value}} megabytes </span>
                            </div>
                            <tooltip placement="right" tooltip="The maximum local file size in MB before all metrics are dropped. Lower bound 2 MB, upper bound 20 MB"> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Flush To AWS </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="150" [max]="1800" [step]="10" [formControl]="settingsForm.controls.file_send_metrics_interval_in_seconds" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.file_send_metrics_interval_in_seconds.value}} seconds </span>
                            </div>
                            <tooltip placement="right" tooltip="The period of time in seconds before flushing the local file to AWS. Lower bound 150 seconds, upper bound 1800 seconds"> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Max Payload Size </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="2" [max]="9" [step]="0.1" [formControl]="settingsForm.controls.file_max_metrics_to_send_in_batch_in_mb" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.file_max_metrics_to_send_in_batch_in_mb.value}} megabytes </span>
                            </div>
                            <tooltip placement="right" tooltip="The maximum size in MB in which we can send to AWS. Lower bound 2 MB, upper bound 9 MB"> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Prioritization Threshold </label>
                            <div class="slider-container">
                                <nouislider [connect]="[true, false]" [min]="20" [max]="100" [step]="5" [formControl]="settingsForm.controls.file_threshold_to_prioritize_in_perc" >
                                </nouislider>
                                <span class="small"> {{settingsForm.controls.file_threshold_to_prioritize_in_perc.value}} % </span>
                            </div>
                            <tooltip placement="right" tooltip="The percentage threshold of the file_max_size_in_mb in which we will start to prioritize events.  Events of lower priority are dropped as local disk space runs out. Lower bound 20 %, upper bound 100 %"> </tooltip>
                        </div>
                        <div class="form-group row">
                            <label class="col-lg-3"> Save Longitude/Latitude </label>
                            <div class="slider-container">
                                <div class="col-2">
                                    <dropdown [options]="booleanOptions" (dropdownChanged)="updateWriteLongLat($event)" [currentOption]="{text:writeLongLat}" name="dropdown" ></dropdown>
                                </div>                                
                            </div>
                            <tooltip placement="right" tooltip="Longitude/Latitude is considered personally identifiable information in some countries and is against the law. It is disabled by default."> </tooltip>
                        </div>
                        <div class="form-group row">
                            <button class="btn l-primary btn-primary" type="submit">
                                Update Settings
                            </button>
                        </div>
                    </form>
                </div>
            </ng-container>
        </div>
        <div class="metric-settings-container extra-actions">
            <h2> Extra Actions </h2>
            <h3> Consume SQS Messages </h3>
            <p>
                This command will invoke a SQS message consumer.  The consumer will aggregate and write the metrics in each SQS message to S3.
            </p>
            <button class="btn btn-outline-primary" (click)="invokeConsumer()"> Consume </button>
            <h3> Amoeba Generator </h3>
            <p>
                This command will invoke the Amoeba generator.  The generator will crawl the S3 bucket and combine multiple small files into a single large S3 file to optimize for Athena consumption.  Files that have gone through this process will have a "_combined" filename suffix.
            </p>
            <button class="btn btn-outline-primary" (click)="invokeAmoeba()"> Unleash Amoeba </button>
            <h3> AWS Glue Crawler </h3>
            <p>
                This command will invoke the GLUE lambda crawler.  The lambda will query the database tables, attempt to recover corrupt partitions, and start the AWS Glue crawler.
            </p>
            <button class="btn btn-outline-primary" (click)="invokeCrawler()"> Crawl </button> 
            <p>The AWS GLUE data crawler is {{crawlerStatus}}.</p>
            
        </div>
    `,
    styles: [`
        .metric-settings-container {
            margin-bottom: 25px;
        }
        .metric-settings-container > h2 {
            margin-bottom: 25px;
        }
        .metric-settings-container.extra-actions > h2 {
            margin-bottom: 15px;
        }
        .form-group.row > label {
            max-width: 275px;
            margin-top: 8px;
        }
        .form-group.row input{
            width: 70px;
        }
        .extra-actions > button {
            margin-bottom: 30px;
        }
        nouislider {
            width: 200px;
            margin-top: 8px;
            margin-right: 10px;
        }
    `]
})

export class MetricSettingsComponent implements OnInit {
    @Input() context: any;
    private _apiHandler: CloudGemMetricApi;
    private settingsForm: FormGroup;
    private isLoadingSettings: boolean;
    private someRange = [1, 50]
    private crawlerStatus: string = "initializing"        
    private booleanOptions = [{ text: "False" }, { text: "True" }]
    private writeLongLat = "False"
    private _crawlerName: string
    private _statusTimeout : any    

    constructor(private http: Http, private aws: AwsService, private fb: FormBuilder, private toastr: ToastsManager) { }

    ngOnInit() {
        this._apiHandler = new CloudGemMetricApi(this.context.ServiceUrl, this.http, this.aws);
        this.isLoadingSettings = true;
        this.getSettings();
        this._crawlerName = (this.aws.context.name.replace("-","_") + "_" + this.aws.context.project.activeDeployment.settings.name.replace("-","_") + "_cloudgemmetric").toLowerCase()
        
        this._statusTimeout = setInterval(() => {
            this.getCrawlerStatus()
        }, 5000);
    }

    ngOnDestroy() {
        // When navigating to another gem remove all the interval timers to stop making requests.        
        clearInterval(this._statusTimeout );        
    }

    getSettings = () => {
        this._apiHandler.getSettings().subscribe( res => {
            let settings = JSON.parse(res.body.text()).result;
            this.settingsForm = this.fb.group({
                growth_rate_trigger_percent: [settings.growth_rate_trigger * 100, Validators.required],
                csv_parquet_compression_ratio: [settings.csv_parquet_compression_ratio, Validators.required],
                backoff_max_seconds: [settings.backoff_max_seconds, Validators.required],
                amoeba_target_aggregation_file_size_in_MB: [settings.amoeba_target_aggregation_file_size_in_MB, Validators.required],
                number_of_initial_consumer_invokes: [settings.number_of_initial_consumer_invokes, Validators.required],
                backoff_base_seconds: [settings.backoff_base_seconds, Validators.required],
                max_inflight_messages: [settings.max_inflight_messages, Validators.required],                
                threshold_before_spawning_new_lambda: [settings.threshold_before_spawning_new_lambda, Validators.required],
                backoff_max_trys: [settings.backoff_max_trys, Validators.required],
                max_message_retry: [settings.max_message_retry, Validators.required],
                csv_seperator: [settings.csv_seperator, Validators.required],
                fifo_limit_before_new_q: [settings.fifo_limit_before_new_q, Validators.required],
                mem_trigger: [settings.mem_trigger, Validators.required],
                amoeba_mem_trigger: [settings.amoeba_mem_trigger, Validators.required],
                max_lambda_execution_time: [settings.max_lambda_execution_time, Validators.required],
                aggregation_period_in_sec: [settings.aggregation_period_in_sec, Validators.required],
                buffer_flush_to_file_interval_sec: [settings.buffer_flush_to_file_interval_sec, Validators.required],
                buffer_flush_to_file_max_in_bytes: [settings.buffer_flush_to_file_max_in_bytes, Validators.required],
                file_max_metrics_to_send_in_batch_in_mb: [settings.file_max_metrics_to_send_in_batch_in_mb, Validators.required],
                file_send_metrics_interval_in_seconds: [settings.file_send_metrics_interval_in_seconds, Validators.required],
                file_max_size_in_mb: [settings.file_max_size_in_mb, Validators.required],
                file_threshold_to_prioritize_in_perc: [settings.file_threshold_to_prioritize_in_perc, Validators.required],
                frequency_to_check_to_spawn: [settings.frequency_to_check_to_spawn, Validators.required],                 
                frequency_to_check_sqs_state: [settings.frequency_to_check_sqs_state, Validators.required]
            });
            this.writeLongLat = settings.write_long_lat
            this.isLoadingSettings = false;
        })
    }

    updateSettings = () => {
        this.isLoadingSettings = true;
        let settingsObj = this.settingsForm.value;
        settingsObj.growth_rate_trigger = (settingsObj.growth_rate_trigger_percent / 100).toString();
        settingsObj.write_long_lat = this.writeLongLat
        delete settingsObj.growth_rate_trigger_percent;
        this._apiHandler.updateSettings(settingsObj).subscribe(() => {
            this.toastr.success("The settings have been updated successfully.");
            this.getSettings();
        }, err => this.toastr.error("Unable to update settings: " + err));
    }

    invokeConsumer = () => {
        this._apiHandler.invokeConsumer().subscribe( () => {
            this.toastr.success("Invoking SQS message consumer");
        }, err => console.log("Unable to invoke SQS consumer: " + err));
    }

    invokeAmoeba = () => {
        this._apiHandler.invokeAmoeba().subscribe( () => {
            this.toastr.success("Invoking amoeba S3 file aggregator");
        }, err => console.log("Unable to invoke amoeba file aggregator: " + err));
    }

    invokeCrawler = () => {
        this._apiHandler.invokeCrawler().subscribe(() => {
            this.toastr.success("Invoking AWS GLUE lambda");            
        }, err => console.log("Unable to invoke GLUE lambda: " + err));
    }

    getCrawlerStatus = () => {
        this._apiHandler.getCrawlerStatus(this._crawlerName).subscribe((r) => {
            let data = JSON.parse(r.body.text()).result;
            this.crawlerStatus = data["Data"]["State"]            
        }, err => console.log("Unable to get GLUE lambda status: " + err))      
    }

    updateWriteLongLat = (val) => {        
        this.writeLongLat = val.text        
    }
}