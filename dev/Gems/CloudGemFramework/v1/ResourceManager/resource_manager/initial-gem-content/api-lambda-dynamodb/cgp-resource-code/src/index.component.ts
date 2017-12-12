import { Input, Component } from '@angular/core';
import { AbstractCloudGemIndexComponent } from 'app/view/game/module/cloudgems/class/index';
import { $-GEM-NAME-$Api } from './index' 
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
 
@Component({
    selector: '$-GEM-NAME-LOWER-CASE-$-index',
	template: `<facet-generator [context]="context" 
        [tabs]="['Overview']"
        (tabClicked)="idx=$event"
        ></facet-generator>
		<div *ngIf="idx == 0 || !idx">Welcome to your Cloud Gem template.</div>
		`
    // OR you can use a 'templateUrl': 'node_modules/@cloud-gems/$-GEM-NAME-LOWER-CASE-$/index.component.html'
})
export class $-GEM-NAME-$IndexComponent extends AbstractCloudGemIndexComponent {
    @Input() context: any;  //REQUIRED
	private _apiHandler: $-GEM-NAME-$Api;
  
    constructor(private http: Http, private aws: AwsService) {
        super()
    }
  
    ngOnInit() {
        this._apiHandler = new $-GEM-NAME-$Api(this.context.ServiceUrl, this.http, this.aws);
    }
    //Your component controller code
}