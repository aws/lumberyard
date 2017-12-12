import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";

/**
 * API Handler for the RESTful services defined in the cloud gem swagger.json
*/
export class $-GEM-NAME-$Api extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws);
    }   
    //custom service api calls here
}
//end rest api handler