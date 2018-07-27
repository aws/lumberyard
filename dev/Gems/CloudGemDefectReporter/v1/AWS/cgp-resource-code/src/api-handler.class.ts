import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Observable } from 'rxjs/Observable';
import { RandomUtil } from './index' ;
import { LyMetricService } from 'app/shared/service/index';

import 'rxjs/add/operator/delay'; // USED for testing, remove in production

/**
 * API Handler for the RESTful services defined in the cloud gem swagger.json
*/
export class CloudGemDefectReporterApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService, metric: LyMetricService = null, metricIdentifier: string = undefined) {
        super(serviceBaseURL, http, aws);
    }   

    public getDummyDefect(): Observable<any> {
        // ** Testing only : Remove these for production **
        // Dummy defect fetching function with delay to test defect details

        let randomTimeOut = RandomUtil.getRandomInt(0,2000);
        console.log("Random Time Out: " + randomTimeOut);

        let fakeResponse = [1];
        let delayedObservable = Observable.of(fakeResponse).delay(randomTimeOut);
                    
        return delayedObservable;        
    }

    public getReportHeaders(): Observable<any> {
        return super.get("headers");
    }

    public updateReportHeader(body: any): Observable<any> {
        return super.put("header", body);
    }

    public getReportComments(universal_unique_identifier): Observable<any> {
        return super.get("comment/" + universal_unique_identifier);
    }

    public updateReportComments(body: any): Observable<any> {
        return super.put("comment", body);
    }

    public updateAddtionalReportInfo(body: any): Observable<any> {
        return super.put("additionalinfo", body);
    }

    public getRecentSearches(user_id: string): Observable<any> {
        return super.get("recentsearches/" + user_id);
    }

    public addNewSearch(body: any): Observable<any> {
        return super.put("recentsearch", body);
    }

    public updateClientConfiguration(body: any): Observable<any> {
        return super.put("clientconfiguration", body);
    }

    public getClientConfiguration(): Observable<any> {
        return super.get("clientconfiguration");
    }
}
//end rest api handler