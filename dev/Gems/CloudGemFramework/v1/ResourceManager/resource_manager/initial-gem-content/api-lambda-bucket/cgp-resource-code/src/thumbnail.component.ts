import { AbstractCloudGemThumbnailComponent, TackableStatus, TackableMeasure, Measurable } from 'app/view/game/module/cloudgems/class/index';
import { Component, Input, OnInit } from '@angular/core'
import { Observable } from 'rxjs/Observable'
 
@Component({
    selector: '$-GEM-NAME-LOWER-CASE-$-thumbnail',
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
export class $-GEM-NAME-$ThumbnailComponent implements AbstractCloudGemThumbnailComponent{
    @Input() context: any
    @Input() displayName: string = "$-GEM-NAME-$";
    @Input() srcIcon: string = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/support_platform._V536715120_.png"
               
    public state: TackableStatus = new TackableStatus();
    public metric: TackableMeasure = new TackableMeasure();
 
    constructor() {
    }
 
    ngOnInit() {       
        this.report(this.metric)
        this.assign(this.state)
    }
 
    public report(metric: Measurable) {
        metric.name = "My Metric";
        metric.value = "1million!";
 
        new Observable<any>(observer => {
            setTimeout(() => {
                observer.next({
                    value: '100,000,000'
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
        status.label = "My Status";
        status.styleType = "Enabled";
 
        new Observable<any>(observer => {
            setTimeout(() => {
                observer.next({
                    status: 'Online'
                });
            }, 3000);
 
            setTimeout(() => {
                observer.complete();
            }, 1000);
        }).subscribe(response => {
            status.label = response.status;
            status.styleType = response.status;
        })
    }
 
}