import { AbstractCloudGemThumbnailComponent, TackableStatus, TackableMeasure, Measurable } from 'app/view/game/module/cloudgems/class/index';
import { Component, Input, OnInit } from '@angular/core'
import { Observable } from 'rxjs/Observable'
import { CloudGemDefectReporterApi } from './index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { LyMetricService } from 'app/shared/service/index';
 
@Component({
    selector: 'cloudgemdefectreporter-thumbnail',
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
export class CloudGemDefectReporterThumbnailComponent implements AbstractCloudGemThumbnailComponent{
    @Input() context: any
    @Input() displayName: string = "Defect Reporter";
    @Input() srcIcon: string = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/defect_reporter._CB1524608681_.png"
               
    public state: TackableStatus = new TackableStatus();
    public metric: TackableMeasure = new TackableMeasure();

    private _apiHandler: CloudGemDefectReporterApi;
 
    constructor(private http: Http, private aws: AwsService, private metricservice: LyMetricService) {
    }
 
    ngOnInit() {
        this._apiHandler = new CloudGemDefectReporterApi(this.context.ServiceUrl, this.http, this.aws, this.metricservice, this.context.identifier);       
        this.report(this.metric)
        this.assign(this.state)
    }
 
    public report(metric: Measurable) {
        metric.name = "";
        metric.value = "";
 
        new Observable<any>(observer => {
            setTimeout(() => {
                observer.next({
                    value: ''
                });
            }, 3000);
 
            setTimeout(() => {
                observer.complete();
            }, 1000);
        }).subscribe(response => {
            metric.value = response.value;
        })
    }
 
    public assign(status: TackableStatus) {
        status.label = "Loading";
        status.styleType = "Loading";

        this._apiHandler.get("service/status").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            status.label = obj.result.status == "online" ? "Online" : "Offline";
            status.styleType = obj.result.status == "online" ? "Enabled" : "Offline";
        }, err => {
            status.label = "Offline";
            status.styleType = "Offline";
        })
    }
 
}