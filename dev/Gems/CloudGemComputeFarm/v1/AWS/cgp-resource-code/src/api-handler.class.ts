import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { AwsService } from "app/aws/aws.service";
import { Observable } from 'rxjs/Observable';
import { LyMetricService } from 'app/shared/service/index';

/**
 * API Handler for the RESTful services defined in the cloud gem swagger.json
*/
export class CloudGemComputeFarmApi extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws);
    }   
    //custom service api calls here
    public getEvents(buildId: string, latestTimestamp: number) : Observable<any> {
        return super.get("events?time=" + latestTimestamp + "&run_id=" + buildId);
    }

    public status(id: string = "") {
        return super.get("status?workflow_id=" + encodeURIComponent(id));
    }
    
    public run(workflowId, body) : Observable<any> {
        return super.post("run", {'workflow_id': workflowId, 'run_params': body});
    }

    public cancel(workflowId) : Observable<any> {
        return super.post("cancel", workflowId);
    }

    public clear() : Observable<any> {
        return super.post("clear");
    }
 
    public getAmis(): Observable<any> {
        return super.get("amis");
    }

    public getKeyPairs(): Observable<any> {
        return super.get("keypairs");
    }

    public createAutoscalingLaunchConfiguration(body): Observable<any> {
        return super.post("autoscaling/config", body);
    }

    public postFleetConfiguration(body): Observable<any> {
        return super.post("fleetconfig", body);
    }

    public getFleetConfiguration(): Observable<any> {
        return super.get("fleetconfig");
    }

    public createOrUpdateAutoscalingGroup(body): Observable<any> {
        return super.post("autoscaling/autoscalinggroup", body);
    }

    public deleteAutoscalingLaunchConfiguration(name): Observable<any> {
        return super.delete("autoscaling/config/" + name);
    }

    public deleteAutoscalingGroup(name): Observable<any> {
        return super.delete("autoscaling/autoscalinggroup/" + name);
    }

    public getBuildIds() : Observable<any> {
        return super.get("builds");
    }
}
//end rest api handler