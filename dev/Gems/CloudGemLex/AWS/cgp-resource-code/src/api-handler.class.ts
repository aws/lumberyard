import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/rx';
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the Speech to Text RESTful services defined in the cloud gem swagger.json
*/
export class SpeechToTextApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService, metricIdentifier: string) {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }

    public getBots(next_token): Observable<any> {
        return super.get("admin/listbots/" + next_token);
    }

    public getDesc(name): Observable<any> {
        return super.get("admin/botdesc/" + name);
    }

    public putDesc(body: any): Observable<any> {
        return super.put("admin/botdesc", body);
    }

    public deleteBot(name): Observable<any> {
        return super.delete("admin/deletebot/" + name);
    }

    public publishBot(name, version): Observable<any> {
        return super.put("admin/publishbot/" + name + "/" + version);
    }

    public getBotStatus(name): Observable<any> {
        return super.get("admin/botstatus/" + name);
    }
}
//end rest api handler