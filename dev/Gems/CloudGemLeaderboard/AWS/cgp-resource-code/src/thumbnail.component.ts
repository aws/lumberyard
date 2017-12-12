import { AbstractCloudGemThumbnailComponent, TackableStatus, TackableMeasure, Measurable } from 'app/view/game/module/cloudgems/class/index';
import { Component, Input, OnInit } from '@angular/core'
import { LeaderboardApi } from './index'
import { Observable } from 'rxjs/Observable'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { LyMetricService } from 'app/shared/service/index';

@Component({
    selector: 'cloudgemleaderboard-thumbnail',
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
export class LeaderboardThumbnailComponent extends AbstractCloudGemThumbnailComponent {
    @Input() context: any
    @Input() displayName: string = "Leaderboard";
    @Input() srcIcon: string = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/Leadboard_icon_optimized._V518452895_.png"
              
    public state: TackableStatus = new TackableStatus();
    public metric: TackableMeasure = new TackableMeasure();

    private _apiHandler: LeaderboardApi;

    constructor(private http: Http, private aws: AwsService, private metricservice: LyMetricService) {
        super()     
    }


    ngOnInit() {        
        this._apiHandler = new LeaderboardApi(this.context.ServiceUrl, this.http, this.aws, this.metricservice, this.context.identifier);
        this.report(this.metric);
        this.assign(this.state);        
    }

    public report(metric: Measurable) {
        metric.name = "Total Leaderboards";
        metric.value = "Loading...";
        this._apiHandler.get("stats").subscribe(response => {
            let obj = JSON.parse(response.body.text());
            metric.value = obj.result.stats.length;
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

}