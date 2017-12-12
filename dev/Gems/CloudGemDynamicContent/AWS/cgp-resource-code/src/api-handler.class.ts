import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/Observable'
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the Message of the Day RESTful services defined in the cloud gem swagger.json
*/
export class DynamicContentApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService = null, metricIdentifier: string = undefined) {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }
    
    public delete(id: string): Observable<any> {
        return super.delete("portal/info/" + id);
    }

    public post(body: any): Observable<any> {
        return super.post("portal/content", body);
    }

    public getContent(): Observable<any> {
        return super.get("portal/content");
    }
}
//end rest api handler