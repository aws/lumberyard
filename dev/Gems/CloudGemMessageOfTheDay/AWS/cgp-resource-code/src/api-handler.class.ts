import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/Observable'
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the Message of the Day RESTful services defined in the cloud gem swagger.json
*/
export class MessageOfTheDayApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService = null, metricIdentifier: string = undefined) {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }

    public delete(id: string): Observable<any> {
        return super.delete("admin/messages/" + id);
    }

    public put(id: string, body: any): Observable<any> {
        return super.put("admin/messages/" + id, body);
    }

    public post(body: any): Observable<any> {
        return super.post("admin/messages/", body);
    }

    public getMessages(filter: string, startIndex: number, count: number): Observable<any> {
        return super.get("admin/messages?count=" + count + "&filter=" +
            filter + "&index=" + startIndex);
    }
}
//end rest api handler