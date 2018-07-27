import { Component, OnInit, OnDestroy, Input } from '@angular/core';
import { CloudGemMetricApi } from './api-handler.class';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Observable } from 'rxjs/Observable';
import { MetricGraph } from 'app/view/game/module/shared/class/metric-graph.class';
import 'rxjs/add/operator/combineLatest'

const DYNAMODB_HARMONIZATION_COEFFICIENT = 0.0083528

@Component({
    selector: 'metric-overview',
    template: `
        <div>Charts will become available as data becomes present.  Charts will refresh automatically every 5 minutes. </div>
        <ng-container>
                <ng-container *ngFor="let chart of metricGraphs">
                    <graph [ref]="chart">                                    
                    </graph>
                </ng-container>
        </ng-container>
    `
})
export class MetricOverviewComponent implements OnInit, OnDestroy{
    @Input('context') public context: any;
    @Input('facetid') public facetid: string
    private _apiHandler: CloudGemMetricApi;
    metricGraphs = new Array<MetricGraph>();
    amoebaGraph: MetricGraph;

    colorScheme = {
        domain: ['#6441A5', '#A10A28', '#C7B42C', '#8A0ECC', '#333333']
    };

    constructor(private http: Http, private aws: AwsService) { }

    ngOnInit() {
        this._apiHandler = new CloudGemMetricApi(this.context.ServiceUrl, this.http, this.aws);
        this.facetid = this.facetid.toLocaleLowerCase()
        if (this.facetid == 'Overview'.toLocaleLowerCase())
            this.populateOverviewGraphs();
        else if (this.facetid == 'SQS'.toLocaleLowerCase())
            this.populatePipelineGraphs();
        else if (this.facetid == 'Lambda'.toLocaleLowerCase())
            this.populateAWSGraphs();
        else if (this.facetid == 'DynamoDb'.toLocaleLowerCase())
            this.populateDbGraphs();

    }

    ngOnDestroy() {
        // When navigating to another gem remove all the interval timers to stop making requests.
        this.metricGraphs.forEach(graph => {
            graph.clearInterval();
        });
    }

    /**
    * PopulateGraphs
    * Creates the multiple graphs on the metrics overview facet
    */
    private populateOverviewGraphs() {        
        this.metricGraphs.push(new MetricGraph("Incoming Game Events", "Date", "Number of Events", ["Metrics Added"], [this._apiHandler.getRowsAdded()], [this.parseMetricsData], "ngx-charts-line-chart", [], ['Average'], undefined, undefined, 300000));            
        this.metricGraphs.push(new MetricGraph("Event Bandwidth Consumed", "Date", "Incoming Bytes", ["Processed Bytes"], [this._apiHandler.getProcessedBytes()], [this.parseMetricsData], "ngx-charts-line-chart", [], ['Average'], undefined, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Event Duplication Error Rate", "Date", "Rate", ["Error Rate"], [this._apiHandler.getDuplicationRate()], [this.parseMetricsData], "ngx-charts-line-chart", [], ['Value'], undefined, undefined, 300000));        
        this.metricGraphs.push(new MetricGraph("Time To Save Events To S3", "Date", "Seconds", ["Save Duration"], [this._apiHandler.getSaveDuration()], [this.parseMetricsData], "ngx-charts-line-chart", [], ['Average'], undefined, undefined, 300000));
    }   

    /**
   * PopulateGraphs
   * Creates the multiple graphs on the metrics overview facet
   */
    private populatePipelineGraphs() {
        this.metricGraphs.push(new MetricGraph("Processed SQS Messages", "Date", "Messages", ["Processed Messages"], [this._apiHandler.getProcessedMessages()], [this.parseMetricsData], "ngx-charts-line-chart", [], ['Average'], undefined, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Time To Delete SQS Messages", "Date", "Seconds", ["Delete Duration"], [this._apiHandler.getDeleteDuration()], [this.parseMetricsData], "ngx-charts-line-chart", [], ['Average'], undefined , undefined, 300000));
    }

    /**
    * PopulateGraphs
    * Creates the multiple graphs on the metrics overview facet
    */
    private populateDbGraphs() {
        this.metricGraphs.push(new MetricGraph("Reads Consumed/Provisioned", "Date", "Average Capacity", ["Consumed", "Provisioned"],
            [this._apiHandler.getCloudWatchMetrics("DynamoDB", "ConsumedReadCapacityUnits", "TableName", "MetricContext", "SampleCount", 60, 8),
                this._apiHandler.getCloudWatchMetrics("DynamoDB", "ProvisionedReadCapacityUnits", "TableName", "MetricContext", "Sum", 60, 8)], [this.parseDynamoDbMetricsData, this.parseMetricsData], "ngx-charts-line-chart", [], ["SampleCount", "Sum"], this.colorScheme, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Writes Consumed/Provisioned", "Date", "Average Capacity", ["Consumed", "Provisioned"],
            [this._apiHandler.getCloudWatchMetrics("DynamoDB", "ConsumedWriteCapacityUnits", "TableName", "MetricContext", "SampleCount", 60, 8),
                this._apiHandler.getCloudWatchMetrics("DynamoDB", "ProvisionedWriteCapacityUnits", "TableName", "MetricContext", "Sum", 60, 8)], [this.parseDynamoDbMetricsData, this.parseMetricsData], "ngx-charts-line-chart", [], ["SampleCount", "Sum"], this.colorScheme, undefined, 300000));
    }


    /**
    * PopulateGraphs
    * Creates the multiple graphs on the metrics overview facet
    */
    private populateAWSGraphs() {
        this.metricGraphs.push(new MetricGraph("Producer Lambda Invocations", "Date", "Invocations", ["Producer Invocations"], [this._apiHandler.getCloudWatchMetrics("Lambda", "Invocations", "FunctionName", "FIFOProducer", "SampleCount", 300, 8)], [this.parseMetricsData], "ngx-charts-line-chart", [], ["SampleCount"], undefined, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Producer Lambda Errors", "Date", "Errors", ["Producer Errors"], [this._apiHandler.getCloudWatchMetrics("Lambda", "Errors", "FunctionName", "FIFOProducer", "Sum", 300, 8)], [this.parseMetricsData], "ngx-charts-line-chart", [], ["Sum"], undefined, undefined, 300000));        
        this.metricGraphs.push(new MetricGraph("Consumer Lambda Invocations", "Date", "Invocations", ["Consumer Invocations"], [this._apiHandler.getCloudWatchMetrics("Lambda", "Invocations", "FunctionName", "FIFOConsumer", "SampleCount", 300, 8)], [this.parseMetricsData], "ngx-charts-line-chart", [], ["SampleCount"], undefined, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Consumer Lambda Duration", "Date", "Milliseconds", ["Consumer Invocations"], [this._apiHandler.getCloudWatchMetrics("Lambda", "Duration", "FunctionName", "FIFOConsumer", "Average", 300, 8)], [this.parseMetricsData], "ngx-charts-line-chart", [], ["Average"], undefined, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Consumer Lambda Errors", "Date", "Errors", ["Consumer Errors"], [this._apiHandler.getCloudWatchMetrics("Lambda", "Errors", "FunctionName", "FIFOConsumer", "Sum", 300, 8)], [this.parseMetricsData], "ngx-charts-line-chart", [], ["Sum"], undefined, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Amoeba Lambda Invocations", "Date", "Invocations",
            ["Amoeba1", "Amoeba2", "Amoeba3", "Amoeba4", "Amoeba5"], [
                this._apiHandler.getCloudWatchMetrics("Lambda", "Invocations", "FunctionName", "Amoeba1", "SampleCount", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Invocations", "FunctionName", "Amoeba2", "SampleCount", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Invocations", "FunctionName", "Amoeba3", "SampleCount", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Invocations", "FunctionName", "Amoeba4", "SampleCount", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Invocations", "FunctionName", "Amoeba5", "SampleCount", 1200, 8)
            ], [this.parseMetricsData, this.parseMetricsData, this.parseMetricsData, this.parseMetricsData, this.parseMetricsData], "ngx-charts-line-chart", [], ["SampleCount", "SampleCount", "SampleCount", "SampleCount", "SampleCount"], this.colorScheme, undefined, 300000));
        this.metricGraphs.push(new MetricGraph("Ameoba Errors", "Date", "Errors",
            ["Amoeba1", "Amoeba2", "Amoeba3", "Amoeba4", "Amoeba5"], [
                this._apiHandler.getCloudWatchMetrics("Lambda", "Errors", "FunctionName", "Amoeba1", "Sum", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Errors", "FunctionName", "Amoeba2", "Sum", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Errors", "FunctionName", "Amoeba3", "Sum", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Errors", "FunctionName", "Amoeba4", "Sum", 1200, 8),
                this._apiHandler.getCloudWatchMetrics("Lambda", "Errors", "FunctionName", "Amoeba5", "Sum", 1200, 8)
            ], [this.parseMetricsData, this.parseMetricsData, this.parseMetricsData, this.parseMetricsData, this.parseMetricsData], "ngx-charts-line-chart", [], ["Sum", "Sum", "Sum", "Sum", "Sum"], this.colorScheme, undefined, 300000));        
        
    }
    

    /**
    * ParseMetricsData
    * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
    * and formats it for the ngx-charts graphing library.
    * @param series_x_label
    * @param series_y_name
    * @param dataSeriesLabel
    * @param response
    */
    parseMetricsData = (series_x_label: string, series_y_name: string, dataSeriesLabel: string, response: any) => {        
        let data = response;

        // At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
        // a single data point.
        if (data.length <= 1 || !(data instanceof Array)) {
            return {
                "name": dataSeriesLabel,
                "series": [
                    {
                        "name": "Insufficient Data Found",
                        "value": 0
                    }
                ]
            }

        }
        return {
            "name": dataSeriesLabel,
            "series": data.sort((a, b) => {
                return +new Date(a.Timestamp * 1000) - +new Date(b.Timestamp * 1000);
            }).map((e) => {
                return this.parseData(e, series_y_name)
            })
        }
    }

    /**
    * ParseDynamoDB metrics
    * A helper method for parsing out the usful inform from a metrics gem REST endpoint and graphing it.  It takes in an array of data
    * and formats it for the ngx-charts graphing library.
    * @param series_x_label
    * @param series_y_name
    * @param dataSeriesLabel
    * @param response
    */
    parseDynamoDbMetricsData = (series_x_label: string, series_y_name: string, dataSeriesLabel: string, response: any) => {
        let data = response;
        // At least 2 points required to create a line graph.  If this data doesn't exist generate a 0 value for
        // a single data point.
        if (data.length <= 1 || !(data instanceof Array)) {
            return {
                "name": dataSeriesLabel,
                "series": [
                    {
                        "name": "Insufficient Data Found",
                        "value": 0
                    }
                ]
            }

        }
        return {
            "name": dataSeriesLabel,
            "series": data.sort((a, b) => {
                return +new Date(a.Timestamp * 1000) - +new Date(b.Timestamp * 1000);
            }).map((e) => {
                return this.parseData(e, series_y_name, true)
            })
        }
    }


    private precisionRound(number, precision): number {
        var factor = Math.pow(10, precision);
        return Math.round(number * factor) / factor;
    }

    private parseData(e, series_y_name, is_dynamo_db = false): any {
        // convert UNIX timestamp (ms => s)
        let date = new Date(e.Timestamp * 1000)
        let day = date.getDate()
        let hour = date.getHours()
        let min = date.getMinutes()
        let s_day = day < 10 ? `0${day}` : day
        let s_hour = hour < 10 ? `0${hour}` : hour
        let s_min = min < 10 ? `0${min}` : min
        return {
            "name": `${s_day} ${s_hour}:${s_min}m`,
            "value": is_dynamo_db ? this.precisionRound(e[series_y_name] * DYNAMODB_HARMONIZATION_COEFFICIENT, 2) : e[series_y_name]
        }
    }

}