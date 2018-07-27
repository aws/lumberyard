import { Tackable, TackableStatus, TackableMeasure, Measurable } from 'app/view/game/module/cloudgems/class/index';
import { Component, Input, OnInit } from '@angular/core'
import { WebCommunicatorApi } from './index'
import { Observable } from 'rxjs/rx'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { LyMetricService } from 'app/shared/service/index';

@Component({
    selector: 'cloudgemwebcommunicator-thumbnail',
    template: `
    <thumbnail-gem 
        [title]="displayName" 
        [cost]="'Low'" 
        [srcIcon]="srcIcon" 
        [metric]="metric" 
        [state]="state" 
        >
    </thumbnail-gem>`
})
export class WebCommunicatorThumbnailComponent implements Tackable {
    @Input() context: any;
    @Input() displayName: string = "Web Communicator";
    @Input() srcIcon: string = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/WebCommunicator_gem_optimized._CB1524589704_.png"

    public state: TackableStatus = new TackableStatus();
    public metric: TackableMeasure = new TackableMeasure();

    private _apiHandler: WebCommunicatorApi;

    constructor(private http: Http, private aws: AwsService, private metricservice: LyMetricService) {
    }


    ngOnInit() {
        this._apiHandler = new WebCommunicatorApi(this.context.ServiceUrl, this.http, this.aws, this.metricservice, this.context.identifier);
        this.report(this.metric);
        this.assign(this.state);
    }

    public report(metric: Measurable) {
        metric.name = "Channels";
        metric.value = "Loading...";
        this._apiHandler.listAllChannels().subscribe(response => {
            let obj = JSON.parse(response.body.text());  
            metric.value = this.getChannelNum(obj.result);
        }, err => {
            metric.value = "Offline";
        });
    }

    public assign(tackableStatus: TackableStatus) {
        tackableStatus.label = "Loading";
        tackableStatus.styleType = "Loading";
        this._apiHandler.get("service/status").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            tackableStatus.label = obj.result.status == "online" ? "Online" : "Offline";
            tackableStatus.styleType = obj.result.status == "online" ? "Enabled" : "Offline";
        }, err => {
            tackableStatus.label = "Offline";
            tackableStatus.styleType = "Offline";
        })
    }

    private getChannelNum(channelList): any {
        let num = 0;
        let simplifiedChannelList = [];
        for (let channel of channelList) {
            if (simplifiedChannelList.indexOf(channel.ChannelName) == -1) {
                simplifiedChannelList.push(channel.ChannelName);
                num++;
            }
        }
        return num;
    }
}