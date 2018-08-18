import { AbstractCloudGemThumbnailComponent, TackableStatus, TackableMeasure, Measurable } from 'app/view/game/module/cloudgems/class/index';
import { Component, Input, OnInit } from '@angular/core'
import { Observable } from 'rxjs/Observable'
import { CloudGemMetricApi } from './api-handler.class';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { MetricQuery } from 'app/view/game/module/shared/class/index';
 
@Component({
    selector: 'cloudgemmetric-thumbnail',
    template: `
    <thumbnail-gem
        [title]="displayName"
        [cost]="'High'"
        [srcIcon]="srcIcon"
        [metric]="metric"
        [state]="state"
        >
    </thumbnail-gem>`
})
export class CloudGemMetricThumbnailComponent implements AbstractCloudGemThumbnailComponent{
    @Input() context: any
    @Input() displayName: string = "Game Metrics";
    @Input() srcIcon: string = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/CloudGemMetric_optimized._CB492341798_.png"

    private _apiHandler: CloudGemMetricApi;
    public state: TackableStatus = new TackableStatus();
    public metric: TackableMeasure = new TackableMeasure();
 
    constructor(private http: Http, private aws: AwsService) { }
    
    ngOnInit() {       
        this._apiHandler = new CloudGemMetricApi(this.context.ServiceUrl, this.http, this.aws);
        this.report(this.metric)
        this.assign(this.state)
    }
 
    public report(metric: Measurable) {
        metric.name = "Metrics written in last hour";
        metric.value = "Loading...";
        let query = new MetricQuery(this.aws, "clientinitcomplete")
                
        this._apiHandler.getRowsAdded().subscribe(response => {
            metric.value = "0"
            let obj = JSON.parse(response.body.text()).result;
            let d = new Date();
            d.setHours(d.getHours() - 1);
            let unixtimestamp = d.getTime() / 1000
            let sum = 0
            for (let idx in obj) {
                let result = obj[idx]
                if (result['Timestamp'] > unixtimestamp) {
                    sum += result['Average']
                }
            }
            metric.value = Math.floor(sum).toString()
        }, err => {
            metric.value = "0";
        })
    }
 
    public assign(status: TackableStatus) {
        status.label = "Loading";
        status.styleType = "Loading";
        this._apiHandler.getStatus().subscribe(response => {
            let obj = JSON.parse(response.body.text());
            status.label = obj.result.status == "online" ? "Online" : "Offline";
            status.styleType = obj.result.status == "online" ? "Enabled" : "Offline";
        }, err => {
            status.label = "Offline";
            status.styleType = "Offline";
        })
    }
 
}