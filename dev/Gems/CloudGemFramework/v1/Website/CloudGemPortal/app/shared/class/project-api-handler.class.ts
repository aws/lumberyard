import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Observable } from "rxjs/Observable";
import { Subject } from 'rxjs/Subject';
import 'rxjs/add/operator/concatAll'
import 'rxjs/add/observable/throw'


/**
 * API Handler for the RESTful services defined in the cloud gem swagger.json
*/
export class ProjectApi extends ApiHandler {

    constructor(private serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws);

    }

    getDashboard(facetid: string): Observable<any> {
        return super.get('dashboard/' + facetid);
    }

    postDashboard(facetid:string, body:any): Observable<any> {
        return super.post('dashboard/' + facetid, body);
    }

    
}