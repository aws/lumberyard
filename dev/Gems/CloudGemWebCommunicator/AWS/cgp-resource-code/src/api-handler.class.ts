import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Measurable, TackableStatus } from 'app/view/game/module/cloudgems/class/index';
import { Observable } from 'rxjs/rx';
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the Speech to Text RESTful services defined in the cloud gem swagger.json
*/
export class WebCommunicatorApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService, metricIdentifier: string) {
        super(serviceBaseURL, http, aws, metric, metricIdentifier);
    }

    /**
     * List all the existing channels
    **/
    public listAllChannels(): Observable<any> {
        return super.get("portal/channel");
    }

    /**
     * List all the existing users
    **/
    public listAllUsers(): Observable<any> {
        return super.get("portal/users");
    }

    /**
     * Set the status of a user, REGISTERED or BANNED
     * @param body the request body that contains the user's client ID and status info
    **/
    public setUserStatus(body): Observable<any> {
        return super.post("portal/users", body);
    }

    /**
     * Register the client
     * @param registrationType the type of the registration, OPENSSL, or WEBSOCKET
    **/
    public register(registrationType): Observable<any> {
        return super.get("portal/registration/" + registrationType);
    }
}
//end rest api handler