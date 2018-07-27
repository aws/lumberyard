import { AbstractCloudGemThumbnailComponent, TackableStatus, TackableMeasure, Measurable } from 'app/view/game/module/cloudgems/class/index';
import { Component, Input, OnInit } from '@angular/core'
import { Observable } from 'rxjs/Observable'
 
@Component({
    selector: 'cloudgemcomputefarm-thumbnail',
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
export class CloudGemComputeFarmThumbnailComponent implements AbstractCloudGemThumbnailComponent{
    @Input() context: any
    @Input() displayName: string = "Compute Farm";
    @Input() srcIcon: string = "https://m.media-amazon.com/images/G/01/cloudcanvas/images/support_platform._V536715120_.png"
               
    public state: TackableStatus = new TackableStatus();
    public metric: TackableMeasure = new TackableMeasure();
 
    constructor() {
    }
 
    ngOnInit() {       
        // TODO: Add a meaningful metric when we have one
        // this.report(this.metric)
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
        }, err => {
            metric.value = "Offline";
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
        }, err => {
            status.label = "Offline";
            status.styleType = "Offline";
        })
    }
 
}