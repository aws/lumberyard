import { Input, Component } from '@angular/core';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { CloudGemMetricApi } from './index'
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";

@Component({
    selector: 'cloudgemmetric-index',
	template: `<facet-generator [context]="context"
        [tabs]="tabs"
        (tabClicked)="idx=$event"
        ></facet-generator>
        <div *ngIf="idx === 0">            
            <metric-overview [facetid]="tabs[idx]" [context]="context"></metric-overview>
        </div>      
        <div *ngIf="idx === 1">
            <metric-overview [facetid]="tabs[idx]" [context]="context"></metric-overview>
        </div>     
        <div *ngIf="idx === 2">
            <metric-overview [facetid]="tabs[idx]" [context]="context"></metric-overview>
        </div>
        <div *ngIf="idx === 3">
            <metric-overview [facetid]="tabs[idx]" [context]="context"></metric-overview>
        </div>
        <div *ngIf="idx === 4">
            <metric-partitions [context]="context"></metric-partitions>
        </div>
        <div *ngIf="idx === 5">
            <metric-filtering [context]="context"></metric-filtering>
        </div>
        <div *ngIf="idx === 6">
            <metric-settings [context]="context"></metric-settings>
        </div>
		`
})
export class CloudGemMetricIndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;  //REQUIRED
    private tabs = ['Overview', 'SQS', 'Lambda', 'DynamoDb', 'Partitions', 'Filtering', 'Settings']

    constructor(private http: Http, private aws: AwsService) {
        super()
    }

    ngOnInit() {
    }
    //Your component controller code
}