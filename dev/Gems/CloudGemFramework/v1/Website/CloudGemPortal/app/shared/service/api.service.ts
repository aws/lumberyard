import { Injectable } from '@angular/core';
import { ApiHandler } from 'app/shared/class/index';
import { Http } from '@angular/http';
import { Observable } from 'rxjs/Observable';
import { AwsService } from 'app/aws/aws.service';
import { Router } from "@angular/router";

@Injectable()
export class ApiService {    
    private projectApi: ProjectServiceAPI;

    constructor(aws: AwsService, http: Http, router: Router) {

    }
}


export class ProjectServiceAPI extends ApiHandler {
    constructor(serviceBaseURL: string, http: Http, aws: AwsService) {
        super(serviceBaseURL, http, aws, null, "Project");
    }    

    public delete(id: string): Observable<any> {
        return super.delete("portal/info/" + id);
    }

    public post(body: any): Observable<any> {
        return super.post("portal/content", body);
    }

    public get(): Observable<any> {
        return super.get("service/status");
    }
}